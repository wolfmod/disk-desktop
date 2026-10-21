/*
 * Диск Ра Град: страница «Настройки» в стиле мобильного «Диска» (QML), вместо серой формы GeneralSettings.
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

#include "gui/ragradsettingspage.h"

#include "common/utility.h"
#include "gui/application.h"
#include "gui/folderman.h"
#include "gui/guiutility.h"
#include "gui/networkinformation.h"
#include "gui/ragradservicedialogs.h"
#include "gui/scheduling/syncscheduler.h"
#include "gui/settingsdialog.h"
#include "libsync/configfile.h"
#include "libsync/theme.h"
#include "resources/jsontheme.h"

#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::Literals::StringLiterals;

namespace OCC {

namespace {
    QString appName()
    {
        return Resources::JsonTheme::instance().applicationName();
    }
}

RagradSettingsPage::RagradSettingsPage(QWidget *parent)
    : QWidget(parent)
    , _quickWidget(new QmlUtils::OCQuickWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_quickWidget);
    _quickWidget->setOCContext(QUrl(u"qrc:/qt/qml/eu/OpenCloud/gui/qml/SettingsPage.qml"_s), this);
}

void RagradSettingsPage::showEvent(QShowEvent *event)
{
    // как GeneralSettings::reloadConfig: путь к программе в автозапуске должен быть актуальным (после обновления)
    if (autostartAvailable() && !autostartLocked()) {
        Utility::setLaunchOnStartup(appName(), Resources::JsonTheme::instance().applicationDisplayName(), autostart());
    }
    Q_EMIT changed();
    QWidget::showEvent(event);
}

bool RagradSettingsPage::autostartAvailable() const
{
    return !(Utility::isWindows() && Utility::isInstalledByStore());
}

bool RagradSettingsPage::autostartLocked() const
{
    return Utility::hasSystemLaunchOnStartup(appName());
}

bool RagradSettingsPage::autostart() const
{
    return autostartLocked() || Utility::hasLaunchOnStartup(appName());
}

void RagradSettingsPage::setAutostart(bool enable)
{
    if (autostartLocked() || enable == autostart()) {
        return;
    }
    Utility::setLaunchOnStartup(appName(), Resources::JsonTheme::instance().applicationDisplayName(), enable);
    Q_EMIT changed();
}

bool RagradSettingsPage::moveToTrashAvailable() const
{
    return Theme::instance()->enableMoveToTrash();
}

bool RagradSettingsPage::moveToTrash() const
{
    return ConfigFile().moveToTrash();
}

void RagradSettingsPage::setMoveToTrash(bool enable)
{
    if (enable == moveToTrash()) {
        return;
    }
    ConfigFile().setMoveToTrash(enable);
    Q_EMIT syncOptionsChanged();
    Q_EMIT changed();
}

bool RagradSettingsPage::syncHiddenFiles() const
{
    return !FolderMan::instance()->ignoreHiddenFiles();
}

void RagradSettingsPage::setSyncHiddenFiles(bool enable)
{
    if (enable == syncHiddenFiles()) {
        return;
    }
    // настройка папки, но хранится общей для всех папок (как в GeneralSettings)
    FolderMan::instance()->setIgnoreHiddenFiles(!enable);
    Q_EMIT changed();
}

bool RagradSettingsPage::meteredAvailable() const
{
    // Windows поддерживает, но Qt об этом не сообщает (QTBUG-118741) — как в NetworkSettings
    return Utility::isWindows() || NetworkInformation::instance()->supports(NetworkInformation::Feature::Metered);
}

bool RagradSettingsPage::pauseWhenMetered() const
{
    return ConfigFile().pauseSyncWhenMetered();
}

void RagradSettingsPage::setPauseWhenMetered(bool enable)
{
    if (enable == pauseWhenMetered()) {
        return;
    }
    ConfigFile().setPauseSyncWhenMetered(enable);
    FolderMan::instance()->scheduler()->setPauseSyncWhenMetered(enable);
    Q_EMIT changed();
}

int RagradSettingsPage::downloadMode() const
{
    return std::clamp(ConfigFile().useDownloadLimit(), -1, 1);
}

void RagradSettingsPage::setDownloadMode(int mode)
{
    ConfigFile().setUseDownloadLimit(std::clamp(mode, -1, 1));
    FolderMan::instance()->setDirtyNetworkLimits();
    Q_EMIT changed();
}

int RagradSettingsPage::downloadLimit() const
{
    return ConfigFile().downloadLimit();
}

void RagradSettingsPage::setDownloadLimit(int limit)
{
    ConfigFile().setDownloadLimit(std::max(1, limit));
    FolderMan::instance()->setDirtyNetworkLimits();
    Q_EMIT changed();
}

int RagradSettingsPage::uploadMode() const
{
    return std::clamp(ConfigFile().useUploadLimit(), -1, 1);
}

void RagradSettingsPage::setUploadMode(int mode)
{
    ConfigFile().setUseUploadLimit(std::clamp(mode, -1, 1));
    FolderMan::instance()->setDirtyNetworkLimits();
    Q_EMIT changed();
}

int RagradSettingsPage::uploadLimit() const
{
    return ConfigFile().uploadLimit();
}

void RagradSettingsPage::setUploadLimit(int limit)
{
    ConfigFile().setUploadLimit(std::max(1, limit));
    FolderMan::instance()->setDirtyNetworkLimits();
    Q_EMIT changed();
}

void RagradSettingsPage::openIgnoredFiles()
{
    if (_ignoreEditor.isNull()) {
        _ignoreEditor = new RagradQmlDialog(QUrl(u"qrc:/qt/qml/eu/OpenCloud/gui/qml/IgnoredFilesPage.qml"_s), new RagradIgnoreList,
            tr("Файлы, которые не синхронизируются"), QSize(620, 640), ocApp()->settingsDialog());
        ocApp()->showSettings();
        _ignoreEditor->open();
    }
}

void RagradSettingsPage::openLog()
{
    if (_logDialog.isNull()) {
        _logDialog = new RagradQmlDialog(QUrl(u"qrc:/qt/qml/eu/OpenCloud/gui/qml/LogSettingsPage.qml"_s), new RagradLogSettings, tr("Журнал работы"),
            QSize(600, 600), ocApp()->settingsDialog());
        ocApp()->showSettings();
        _logDialog->open();
    }
}

}
