/*
 * Диск Ра Град: пункт «Скопировать ссылку на Диске» в меню Проводника (как у Яндекс.Диска).
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

#include "gui/ragradshell.h"

#include "gui/accountstate.h"
#include "gui/application.h"
#include "gui/folder.h"
#include "gui/folderman.h"
#include "gui/notifications/systemnotification.h"
#include "gui/notifications/systemnotificationmanager.h"
#include "libsync/account.h"
#include "libsync/common/syncjournaldb.h"
#include "libsync/configfile.h"
#include "libsync/networkjobs/jsonjob.h"
#include "libsync/networkjobs/simplenetworkjob.h"
#include "libsync/vfs/vfs.h"
#include "resources/fonticon.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <algorithm>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

namespace OCC::Ragrad {

Q_LOGGING_CATEGORY(lcRagradShell, "gui.ragrad.shell", QtInfoMsg)

namespace {
    // id ресурса движка: <storage>$<space>!<node>; у открытой человеку папки в «общих» после «!» — номер доступа
    const QRegularExpression &itemIdRe()
    {
        static const QRegularExpression re(u"^[0-9a-f-]{36}\\$[0-9a-f-]{36}!\\S+$"_s);
        return re;
    }

    void notify(const QString &title, const QString &text)
    {
        SystemNotificationRequest request(title, text, Resources::FontIcon(QChar(0xf0c1)));
        ocApp()->systemNotificationManager()->notify(std::move(request));
    }

    QString failTitle()
    {
        return QObject::tr("Ссылка не создана");
    }

#ifdef Q_OS_WIN
    const QString &verbKeyName()
    {
        static const QString name = u"RagradDiskCopyLink"_s;
        return name;
    }

    bool setValue(const QString &key, const QString &name, const QString &value)
    {
        const std::wstring k = key.toStdWString();
        const std::wstring n = name.toStdWString();
        const std::wstring v = value.toStdWString();
        const LSTATUS r = RegSetKeyValueW(
            HKEY_CURRENT_USER, k.c_str(), name.isEmpty() ? nullptr : n.c_str(), REG_SZ, v.c_str(), static_cast<DWORD>((v.size() + 1) * sizeof(wchar_t)));
        if (r != ERROR_SUCCESS) {
            qCWarning(lcRagradShell) << u"не записано в реестр" << key << name << r;
        }
        return r == ERROR_SUCCESS;
    }

    QString value(const QString &key, const QString &name)
    {
        const std::wstring k = key.toStdWString();
        const std::wstring n = name.toStdWString();
        wchar_t buf[8192];
        DWORD size = sizeof(buf);
        if (RegGetValueW(HKEY_CURRENT_USER, k.c_str(), name.isEmpty() ? nullptr : n.c_str(), RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS) {
            return {};
        }
        return QString::fromWCharArray(buf);
    }

    void removeKey(const QString &key)
    {
        const std::wstring k = key.toStdWString();
        RegDeleteTreeW(HKEY_CURRENT_USER, k.c_str());
    }
#endif

    /// пункт меню Проводника: только в папках разделов Диска (условие AppliesTo из путей папок)
    void updateExplorerMenu()
    {
#ifdef Q_OS_WIN
        QStringList conditions;
        for (auto *folder : FolderMan::instance()->folders()) {
            QString root = QDir::toNativeSeparators(QDir::cleanPath(folder->path()));
            if (root.isEmpty() || root.contains(u'"')) {
                continue;
            }
            if (!root.endsWith(u'\\')) {
                root += u'\\';
            }
            conditions << u"System.ParsingPath:~<\"%1\""_s.arg(root);
        }
        conditions.removeDuplicates();
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        const QString appliesTo = conditions.join(u" OR "_s);
        const QString command = u"\"%1\" --ragrad-copy-link \"%2\""_s.arg(exe, u"%1"_s);
        for (const QString &cls : {u"*"_s, u"Directory"_s}) {
            const QString key = u"Software\\Classes\\%1\\shell\\%2"_s.arg(cls, verbKeyName());
            if (conditions.isEmpty()) {
                removeKey(key);
                continue;
            }
            if (value(key, u"AppliesTo"_s) == appliesTo && value(key + u"\\command"_s, {}) == command) {
                continue;
            }
            setValue(key, u"MUIVerb"_s, QObject::tr("Скопировать ссылку на Диске"));
            setValue(key, u"Icon"_s, u"\"%1\",0"_s.arg(exe));
            setValue(key, u"AppliesTo"_s, appliesTo);
            setValue(key, u"MultiSelectModel"_s, u"Single"_s);
            setValue(key + u"\\command"_s, {}, command);
            qCInfo(lcRagradShell) << u"пункт меню Проводника записан" << key << appliesTo;
        }
#endif
    }

    // ---- 4.0.11: значок «Есть ссылка» (цепочка) в колонке «Состояние» Проводника ----
    // Сервер отдаёт id файлов и папок с действующими ссылками для просмотра (GET disk-auth/link-marks, только id, после входа).
    // Программа находит их в своём журнале и ставит значок; у тех, что были отмечены раньше и пропали из списка, — снимает.
    // Отмеченные id запоминаются по каждой папке (группа RagradLinkMarks в настройках), чтобы снять значок и после перезапуска.
    QString linkMarksKey(Folder *folder)
    {
        return QString::fromLatin1(QCryptographicHash::hash(folder->path().toUtf8(), QCryptographicHash::Sha1).toHex());
    }

    /// false — файла с таким id в этой папке нет
    bool markById(Folder *folder, const QString &id, bool on)
    {
        bool found = false;
        folder->journalDb()->getFileRecordsByFileId(id.toUtf8(), [&](const SyncJournalFileRecord &rec) {
            if (!rec.isValid() || rec.path().isEmpty()) {
                return;
            }
            found = true;
            folder->vfs().setRagradLinkMark(rec.path(), rec.isDirectory(), on);
        });
        return found;
    }

    void applyLinkMarks(AccountState *state, const QSet<QString> &ids)
    {
        auto settings = ConfigFile::makeQSettings();
        settings.beginGroup(u"RagradLinkMarks"_s);
        for (auto *folder : FolderMan::instance()->folders()) {
            if (folder->accountState().data() != state || !folder->journalDb()) {
                continue;
            }
            const QString key = linkMarksKey(folder);
            const QStringList prev = settings.value(key).toStringList();
            QStringList now;
            for (const QString &id : ids) {
                if (markById(folder, id, true)) {
                    now << id;
                }
            }
            int removed = 0;
            for (const QString &id : prev) {
                if (!ids.contains(id) && markById(folder, id, false)) {
                    ++removed;
                }
            }
            now.sort();
            if (now != prev) {
                qCInfo(lcRagradShell) << u"значки ссылок" << folder->path() << u"отмечено" << now.size() << u"снято" << removed;
            }
            settings.setValue(key, now);
        }
    }

    void refreshLinkMarks()
    {
        QList<QPointer<AccountState>> states;
        for (auto *folder : FolderMan::instance()->folders()) {
            AccountState *st = folder->accountState().data();
            if (st && std::none_of(states.cbegin(), states.cend(), [st](const QPointer<AccountState> &x) { return x.data() == st; })) {
                states << QPointer<AccountState>(st);
            }
        }
        for (const auto &st : states) {
            if (!st || !st->isConnected()) {
                continue;
            }
            const AccountPtr account = st->account();
            auto *job = new JsonJob(account, account->url(), u"disk-auth/link-marks"_s, "GET", SimpleNetworkJob::UrlQuery{}, QNetworkRequest{}, nullptr);
            job->setTimeout(30s);
            QObject::connect(job, &JsonJob::finishedSignal, qApp, [job, st] {
                if (!st) {
                    return;
                }
                if (job->httpStatusCode() != 200) {
                    qCInfo(lcRagradShell) << u"список ссылок не получен" << job->httpStatusCode();
                    return;
                }
                QSet<QString> ids;
                const QJsonArray items = job->data().value("items"_L1).toArray();
                for (const auto &v : items) {
                    const QString id = v.toString();
                    if (itemIdRe().match(id).hasMatch()) {
                        ids.insert(id);
                    }
                }
                applyLinkMarks(st.data(), ids);
            });
            job->start();
        }
    }

    void requestLink(
        QPointer<AccountState> state, QPointer<Folder> folder, const QString &relPath, const QString &itemId, const QString &name, bool isDir, int attempt)
    {
        if (!state) {
            notify(failTitle(), QObject::tr("Нет связи с сервером Диска. Повторите попытку позже."));
            return;
        }
        // программа только что запущена из меню или связь пропала — ждём подключения до минуты
        if (!state->isConnected()) {
            if (attempt >= 30) {
                notify(failTitle(), QObject::tr("Нет связи с сервером Диска. Проверьте подключение к интернету и повторите."));
                return;
            }
            QTimer::singleShot(
                2s, qApp, [state, folder, relPath, itemId, name, isDir, attempt] { requestLink(state, folder, relPath, itemId, name, isDir, attempt + 1); });
            return;
        }
        const AccountPtr account = state->account();
        const QString drive = itemId.left(itemId.indexOf(u'!'));
        // SimpleNetworkJob, а не JsonJob: JsonJob при ошибке не читает тело, а в нём пояснение сервера («нет прав» и т.п.)
        auto *job = new SimpleNetworkJob(account, account->url(), u"graph/v1beta1/drives/%1/items/%2/createLink"_s.arg(drive, itemId), "POST",
            QJsonObject{{u"type"_s, u"view"_s}}, QNetworkRequest{}, nullptr);
        job->setTimeout(60s);
        QObject::connect(job, &SimpleNetworkJob::finishedSignal, qApp, [job, account, folder, relPath, name, isDir] {
            const QJsonObject data = job->reply() ? QJsonDocument::fromJson(job->reply()->readAll()).object() : QJsonObject();
            const QString webUrl = data.value("link"_L1).toObject().value("webUrl"_L1).toString();
            const QString token = webUrl.section(u'/', -1, -1, QString::SectionSkipEmpty);
            if (job->httpStatusCode() == 200 && !token.isEmpty()) {
                QUrl link = account->url();
                link.setPath(u"/s/"_s + token);
                link.setQuery(QString());
                link.setFragment(QString());
                QGuiApplication::clipboard()->setText(link.toString());
                // значок «Есть ссылка» сразу; список с сервера подтвердит его, когда журнал Диска запишет ссылку (раз в минуту)
                if (folder) {
                    folder->vfs().setRagradLinkMark(relPath, isDir, true);
                }
                QTimer::singleShot(75s, qApp, [] { refreshLinkMarks(); });
                qCInfo(lcRagradShell) << u"ссылка скопирована" << name;
                notify(QObject::tr("Ссылка скопирована"),
                    (isDir ? QObject::tr("Ссылка на папку «%1» скопирована. Любой, у кого она есть, сможет открыть и скачать файлы.")
                           : QObject::tr("Ссылка на файл «%1» скопирована. Любой, у кого она есть, сможет открыть и скачать файл."))
                        .arg(name));
                return;
            }
            // сервер объясняет по-русски (нет прав, нет доступа) — показываем его текст, иначе общий
            const QString message = data.value("error"_L1).toObject().value("message"_L1).toString();
            static const QRegularExpression cyr(u"[А-Яа-яЁё]"_s);
            qCWarning(lcRagradShell) << u"ссылка не создана" << name << job->httpStatusCode() << message;
            notify(failTitle(), cyr.match(message).hasMatch() ? message : QObject::tr("Не удалось создать ссылку. Повторите попытку позже."));
        });
        job->start();
    }
}

void startExplorerMenu()
{
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    updateExplorerMenu();
    QObject::connect(FolderMan::instance(), &FolderMan::folderListChanged, qApp, [] { updateExplorerMenu(); });

    // 4.0.11: значок «Есть ссылка» — сверка со списком ссылок раз в 5 минут (первая — через 45 с после запуска)
    auto *linkMarksTimer = new QTimer(qApp);
    linkMarksTimer->setInterval(5min);
    QObject::connect(linkMarksTimer, &QTimer::timeout, qApp, [] { refreshLinkMarks(); });
    linkMarksTimer->start();
    QTimer::singleShot(45s, qApp, [] { refreshLinkMarks(); });
}

void copyPublicLink(const QString &localPath)
{
    const QString clean = QDir::cleanPath(QDir::fromNativeSeparators(localPath));
    const QFileInfo info(clean);
    const QString name = info.fileName();
    QString relPath;
    Folder *folder = FolderMan::instance()->folderForPath(clean, &relPath);
    if (!folder) {
        notify(failTitle(), QObject::tr("«%1» не находится в папке Диска.").arg(name));
        return;
    }
    if (relPath.isEmpty()) {
        notify(failTitle(), QObject::tr("Ссылку можно создать на папку или файл внутри раздела «%1».").arg(name));
        return;
    }
    const auto record = folder->journalDb()->getFileRecord(relPath);
    const QString itemId = QString::fromUtf8(record.fileId());
    if (!record.isValid() || !itemIdRe().match(itemId).hasMatch()) {
        notify(failTitle(),
            (info.isDir() ? QObject::tr("Папка «%1» ещё не загружена на Диск. Дождитесь окончания синхронизации и повторите.")
                          : QObject::tr("Файл «%1» ещё не загружен на Диск. Дождитесь окончания синхронизации и повторите."))
                .arg(name));
        return;
    }
    requestLink(QPointer<AccountState>(folder->accountState().data()), QPointer<Folder>(folder), relPath, itemId, name, record.isDirectory(), 0);
}

}
