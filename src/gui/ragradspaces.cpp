/*
 * Диск Ра Град: какие разделы Диска подключать и куда их класть — как в мобильном приложении.
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

#include "gui/ragradspaces.h"

#include "common/filesystembase.h"
#include "gui/folder.h"
#include "gui/folderman.h"
#include "libsync/account.h"
#include "libsync/graphapi/spacesmanager.h"
#include "libsync/vfs/vfs.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QUrl>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace Qt::Literals::StringLiterals;

namespace OCC::Ragrad {

Q_LOGGING_CATEGORY(lcRagradSpaces, "gui.ragrad.spaces", QtInfoMsg)

namespace {
    // «общие» (Shares) у движка — один и тот же id (см. libsync/graphapi/space.cpp)
    const auto sharesIdC = u"a0ca6a90-a365-4782-871e-d44447bbc668$a0ca6a90-a365-4782-871e-d44447bbc668"_s;
    const auto objectsC = u"Объекты"_s;

    /// путь раздела внутри «Диск Ра Град»: описание без «Диск/» и «Ра Град/» (как в мобильном), иначе имя
    QString projectPath(const GraphApi::Space *space)
    {
        const auto drive = space->drive();
        QString path = drive.getDescription().trimmed();
        static const QRegularExpression diskPrefix(u"^Диск(/|$)"_s);
        static const QRegularExpression companyPrefix(u"^Ра Град(/|$)"_s);
        static const QRegularExpression edgeSlashes(u"^/+|/+$"_s);
        path.remove(diskPrefix);
        path.remove(edgeSlashes);
        path.remove(companyPrefix);
        path.remove(edgeSlashes);
        if (path.isEmpty()) {
            path = drive.getName().trimmed();
        }
        return path;
    }

    bool alreadyConnected(const AccountStatePtr &accountState, const GraphApi::Space *space)
    {
        const auto &folders = FolderMan::instance()->folders();
        return std::any_of(folders.cbegin(), folders.cend(), [&](Folder *f) { return f->accountState() == accountState && f->space() == space; });
    }

    bool useVfsByDefault()
    {
        return VfsPluginManager::instance().bestAvailableVfsMode() != Vfs::Mode::Off;
    }

    /// сколько обновлений списка разделов подряд раздела папки нет у движка (путь папки → счётчик)
    QHash<QString, int> &missingCounts()
    {
        static QHash<QString, int> counts;
        return counts;
    }

    /// «общие» (открытые человеку папки) не отключаем никогда — их набор меняется сам
    bool isSharesFolder(Folder *folder)
    {
        return QUrl::fromPercentEncoding(folder->webDavUrl().toString().toUtf8()).contains(sharesIdC);
    }

    /// есть ли в папке что-то, кроме незагруженных заглушек (скачанные или созданные на компьютере файлы)
    bool hasLocalContent(Folder *folder)
    {
        QDirIterator it(folder->path(), QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            const QString name = QFileInfo(file).fileName();
            if (name.startsWith(".sync_"_L1) || name.compare("Desktop.ini"_L1, Qt::CaseInsensitive) == 0) {
                continue; // служебные файлы программы
            }
            if (!folder->vfs().isDehydratedPlaceholder(file)) {
                return true;
            }
        }
        return false;
    }
}

static bool disconnectGoneSpaces(const AccountStatePtr &accountState)
{
    if (!accountState->isConnected()) {
        return false;
    }
    auto *spacesManager = accountState->account()->spacesManager();
    const auto spaces = spacesManager->spaces();
    // в ответе движка нет ни одного раздела — похоже на сбой, ничего не трогаем
    if (std::none_of(spaces.cbegin(), spaces.cend(), [](const GraphApi::Space *s) { return !s->disabled() && s->drive().getDriveType() == "project"_L1; })) {
        return false;
    }

    QVector<Folder *> gone;
    int ours = 0;
    for (Folder *f : FolderMan::instance()->folders()) {
        if (f->accountState() != accountState || isSharesFolder(f)) {
            continue;
        }
        ++ours;
        const auto *space = f->space();
        if (space && !space->disabled()) {
            missingCounts().remove(f->path());
            continue;
        }
        // раздела нет в двух обновлениях подряд (~30–60 с) — не случайный сбой ответа
        if (++missingCounts()[f->path()] >= 2) {
            gone.append(f);
        }
    }
    if (gone.isEmpty()) {
        return false;
    }
    // больше половины папок разом — похоже на сбой, а не на снятые доступы
    if (gone.size() * 2 > ours) {
        qCWarning(lcRagradSpaces) << u"пропало слишком много разделов сразу, папки не отключаю:" << gone.size() << u"из" << ours;
        return false;
    }

    for (Folder *f : std::as_const(gone)) {
        const QString path = f->path();
        const QString name = f->displayName();
        const bool keepLocal = !f->isReady() || hasLocalContent(f);
        missingCounts().remove(path);
        qCInfo(lcRagradSpaces) << u"раздел больше недоступен, отключаю папку" << name << path;
        FolderMan::instance()->removeFolder(f);
        if (keepLocal) {
            // на компьютере есть скачанные или свои файлы — папку оставляем как обычную (без подписи Диска)
            clearFolderLabel(path);
            qCInfo(lcRagradSpaces) << u"в папке есть файлы на компьютере — папка оставлена" << path;
        } else if (QDir(path).removeRecursively()) {
            qCInfo(lcRagradSpaces) << u"папка без скачанных файлов удалена" << path;
        } else {
            qCWarning(lcRagradSpaces) << u"не удалось удалить папку" << path;
        }
    }
    return true;
}

bool connectSpaces(const AccountStatePtr &accountState, bool useVfs)
{
    const auto account = accountState->account();
    if (!account->hasDefaultSyncRoot()) {
        return false;
    }
    const QString root = account->defaultSyncRoot();
    const auto spaces = account->spacesManager()->spaces();

    // участник «Объектов» видит раздел целиком; иначе у него только папки, открытые по годам (в «общих»)
    const bool objectsMember = std::any_of(spaces.cbegin(), spaces.cend(),
        [](const GraphApi::Space *s) { return !s->disabled() && s->drive().getDriveType() == "project"_L1 && projectPath(s) == objectsC; });

    bool added = false;
    for (auto *space : spaces) {
        if (space->disabled() || alreadyConnected(accountState, space)) {
            continue;
        }
        QString relPath;
        if (space->drive().getDriveType() == "project"_L1) {
            relPath = projectPath(space);
        } else if (space->id() == sharesIdC) {
            relPath = objectsMember ? QObject::tr("Открытые мне") : objectsC;
        } else {
            // личное пространство и прочее не подключаем — как в мобильном приложении
            continue;
        }

        const QFileInfo target(QDir(root).filePath(relPath));
        const QString parentDir = target.absolutePath();
        if (!QDir().mkpath(parentDir)) {
            qCWarning(lcRagradSpaces) << u"cannot create" << parentDir;
            continue;
        }
        const QString localPath =
            FolderMan::findGoodPathForNewSyncFolder(parentDir, target.fileName(), FolderMan::NewFolderType::SpacesFolder, account->uuid());

        auto definition = FolderDefinition{account->uuid(), QUrl(space->drive().getRoot().getWebDavUrl()), space->drive().getRoot().getId(), target.fileName()};
        definition.setLocalPath(localPath);
        if (auto *folder = FolderMan::instance()->addFolderFromWizard(accountState, std::move(definition), useVfs)) {
            folder->setPriority(space->priority());
            added = true;
            qCInfo(lcRagradSpaces) << u"connected space" << space->displayName() << u"->" << localPath << u"vfs" << useVfs;
        }
    }
    return added;
}

void watchSpaces(const AccountStatePtr &accountState)
{
    auto *spacesManager = accountState->account()->spacesManager();
    // AccountStatePtr — QPointer: обнулится сам, если аккаунт удалят
    QObject::connect(spacesManager, &GraphApi::SpacesManager::updated, accountState.data(), [accountState] {
        if (!accountState || accountState->isSettingUp() || !accountState->account()->hasDefaultSyncRoot()) {
            return;
        }
        // 4.0.3: раздел, который человеку больше недоступен (доступ к нему сняли), — отключить, а не показывать ошибку
        bool changed = disconnectGoneSpaces(accountState);
        if (connectSpaces(accountState, useVfsByDefault())) {
            changed = true;
        }
        if (changed) {
            FolderMan::instance()->scheduleAllFolders();
        }
    });
}

void clearFolderLabel(const QString &path)
{
#ifdef Q_OS_WIN
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    // Desktop.ini пишет Folder::prepareFolder (скрытый + системный) — снять атрибуты, иначе файл не удалить
    const QString iniPath = QDir(path).filePath(u"Desktop.ini"_s);
    if (QFileInfo::exists(iniPath)) {
        const QString longIniPath = FileSystem::longWinPath(iniPath);
        SetFileAttributesW(reinterpret_cast<const wchar_t *>(longIniPath.utf16()), FILE_ATTRIBUTE_NORMAL);
        if (!QFile::remove(iniPath)) {
            qCWarning(lcRagradSpaces) << u"не удалось удалить" << iniPath;
        }
    }
    // без системного атрибута Проводник не читает Desktop.ini — показывает настоящее имя папки
    const QString longPath = FileSystem::longWinPath(path);
    const DWORD attrs = GetFileAttributesW(reinterpret_cast<const wchar_t *>(longPath.utf16()));
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_SYSTEM)) {
        SetFileAttributesW(reinterpret_cast<const wchar_t *>(longPath.utf16()), attrs & ~FILE_ATTRIBUTE_SYSTEM);
    }
    qCInfo(lcRagradSpaces) << u"снята подпись Диска с папки" << path;
#else
    Q_UNUSED(path)
#endif
}

} // namespace OCC::Ragrad
