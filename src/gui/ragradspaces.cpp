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
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>

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
        if (connectSpaces(accountState, useVfsByDefault())) {
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
