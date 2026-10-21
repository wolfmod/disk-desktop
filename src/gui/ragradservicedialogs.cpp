/*
 * Диск Ра Град: служебные окна в стиле мобильного «Диска» (QML) вместо серых форм апстрима.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "gui/ragradservicedialogs.h"

#include "gui/folderman.h"
#include "gui/logbrowser.h"
#include "gui/qmlutils.h"
#include "gui/scheduling/syncscheduler.h"
#include "libsync/configfile.h"
#include "libsync/logger.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>

using namespace Qt::Literals::StringLiterals;

namespace OCC {

Q_LOGGING_CATEGORY(lcRagradServiceDialogs, "gui.ragrad.servicedialogs", QtInfoMsg)

RagradQmlDialog::RagradQmlDialog(const QUrl &page, QObject *context, const QString &title, const QSize &size, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setAttribute(Qt::WA_DeleteOnClose);
    context->setParent(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *widget = new QmlUtils::OCQuickWidget(this);
    layout->addWidget(widget);
    widget->setOCContext(page, nullptr, context, QJSEngine::CppOwnership);

    // у объекта страницы есть сигнал finished() — «Закрыть»/«Сохранить»/«Отмена»
    connect(context, SIGNAL(finished()), this, SLOT(close()));
    resize(size);
}

// ---------- «Файлы, которые не синхронизируются» ----------

RagradIgnoreList::RagradIgnoreList(QObject *parent)
    : QObject(parent)
{
    // как IgnoreListEditor: служебные базы синхронизации + встроенный список программы
    _system = {u".csync_journal.db*"_s, u"._sync_*.db*"_s, u".sync_*.db*"_s};
    QFile file(ConfigFile().excludeFile(ConfigFile::SystemScope));
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = QString::fromUtf8(file.readLine());
            while (line.endsWith(u'\n') || line.endsWith(u'\r')) {
                line.chop(1);
            }
            if (line.isEmpty() || line.startsWith(u'#')) {
                continue;
            }
            if (line.startsWith(u']')) {
                line = line.mid(1);
            }
            _system.append(line);
        }
    }
    readUserFile();
}

void RagradIgnoreList::readUserFile()
{
    QFile file(ConfigFile().excludeFile(ConfigFile::UserScope));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QStringList skipped;
    bool readOnly = false;
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());
        while (line.endsWith(u'\n') || line.endsWith(u'\r')) {
            line.chop(1);
        }
        // пустые строки и комментарии сохраняем; «#!readonly» запрещает правку следующей маски
        if (line.isEmpty() || line.startsWith(u'#')) {
            skipped.append(line);
            if (line == "#!readonly"_L1) {
                readOnly = true;
            }
            continue;
        }
        Entry entry;
        if (line.startsWith(u']')) {
            entry.deletable = true;
            line = line.mid(1);
        } else if (line.startsWith("\\#"_L1)) {
            line = line.mid(1);
        }
        entry.pattern = line;
        entry.readOnly = readOnly;
        entry.skippedLines = skipped;
        _user.append(entry);
        skipped.clear();
        readOnly = false;
    }
    _trailingLines = skipped;
}

QVariantList RagradIgnoreList::userPatterns() const
{
    QVariantList list;
    for (const auto &entry : _user) {
        list.append(QVariantMap{{u"pattern"_s, entry.pattern}, {u"deletable"_s, entry.deletable}, {u"readOnly"_s, entry.readOnly}});
    }
    return list;
}

QString RagradIgnoreList::add(const QString &pattern)
{
    const QString value = pattern.trimmed();
    if (value.isEmpty()) {
        return tr("Введите маску, например *.tmp");
    }
    if (value.contains(u'\n') || value.contains(u'\r')) {
        return tr("Маска должна быть в одну строку.");
    }
    const bool exists = _system.contains(value) || std::any_of(_user.cbegin(), _user.cend(), [&value](const Entry &e) { return e.pattern == value; });
    if (exists) {
        return tr("Такая маска уже есть.");
    }
    _user.append(Entry{value, false, false, {}});
    _dirty = true;
    _errorText.clear();
    Q_EMIT changed();
    return {};
}

void RagradIgnoreList::remove(int index)
{
    if (index < 0 || index >= _user.size() || _user.at(index).readOnly) {
        return;
    }
    _user.removeAt(index);
    _dirty = true;
    Q_EMIT changed();
}

void RagradIgnoreList::setDeletable(int index, bool deletable)
{
    if (index < 0 || index >= _user.size() || _user.at(index).readOnly || _user.at(index).deletable == deletable) {
        return;
    }
    _user[index].deletable = deletable;
    _dirty = true;
    Q_EMIT changed();
}

void RagradIgnoreList::save()
{
    if (!_dirty) {
        Q_EMIT finished();
        return;
    }
    // запись — как IgnoreListEditor::slotUpdateLocalIgnoreList
    const QString path = ConfigFile().excludeFile(ConfigFile::UserScope);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcRagradServiceDialogs) << u"cannot write the ignore list" << path << file.errorString();
        _errorText = tr("Не удалось сохранить изменения. Закройте окно и попробуйте ещё раз.");
        Q_EMIT changed();
        return;
    }
    for (const auto &entry : std::as_const(_user)) {
        for (const auto &line : entry.skippedLines) {
            file.write(line.toUtf8() + '\n');
        }
        QByteArray prefix;
        if (entry.deletable) {
            prefix = "]";
        } else if (entry.pattern.startsWith(u'#')) {
            prefix = "\\";
        }
        file.write(prefix + entry.pattern.toUtf8() + '\n');
    }
    for (const auto &line : std::as_const(_trailingLines)) {
        file.write(line.toUtf8() + '\n');
    }
    file.close();

    // после смены списка — полный обход: иначе файлы, которые больше не исключены, не скачаются (issue #3172)
    auto *folderMan = FolderMan::instance();
    for (auto *folder : folderMan->folders()) {
        if (folder->isReady()) {
            folder->journalDb()->forceRemoteDiscoveryNextSync();
            folder->reloadExcludes();
            folder->slotNextSyncFullLocalDiscovery();
            folderMan->scheduler()->enqueueFolder(folder);
        }
    }
    _dirty = false;
    Q_EMIT changed();
    Q_EMIT finished();
}

void RagradIgnoreList::cancel()
{
    Q_EMIT finished();
}

// ---------- «Журнал работы» ----------

bool RagradLogSettings::enabled() const
{
    return ConfigFile().automaticLogDir();
}

void RagradLogSettings::setEnabled(bool enabled)
{
    if (enabled == this->enabled()) {
        return;
    }
    ConfigFile().setAutomaticLogDir(enabled);
    LogBrowser::setupLoggingFromConfig();
    Q_EMIT changed();
}

bool RagradLogSettings::httpLog() const
{
    return ConfigFile().logHttp();
}

void RagradLogSettings::setHttpLog(bool enabled)
{
    if (enabled == httpLog()) {
        return;
    }
    ConfigFile().configureHttpLogging(std::make_optional(enabled));
    Q_EMIT changed();
}

// как в LogBrowser: не меньше 5 (Logger хранит минимум 5; ненастроенное значение — 0)
int RagradLogSettings::keepCount() const
{
    return std::clamp(ConfigFile().automaticDeleteOldLogs(), 5, 99);
}

void RagradLogSettings::setKeepCount(int count)
{
    count = std::clamp(count, 5, 99);
    if (count == keepCount()) {
        return;
    }
    ConfigFile().setAutomaticDeleteOldLogs(count);
    Logger::instance()->setMaxLogFiles(count);
    Q_EMIT changed();
}

void RagradLogSettings::openFolder()
{
    const QString path = Logger::instance()->temporaryFolderLogDirPath();
    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void RagradLogSettings::close()
{
    Q_EMIT finished();
}

} // namespace OCC
