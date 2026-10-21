/*
 * Диск Ра Град: страница «Настройки» в стиле мобильного «Диска» (QML), вместо серой формы GeneralSettings.
 * Настройки те же и хранятся там же (ConfigFile, автозапуск Windows); язык не выбирается — берётся из системы.
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

#pragma once

#include "gui/qmlutils.h"

#include <QDialog>
#include <QPointer>
#include <QWidget>
#include <QtQml/qqmlregistration.h>

namespace OCC {

class RagradSettingsPage : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autostartAvailable READ autostartAvailable CONSTANT)
    Q_PROPERTY(bool autostartLocked READ autostartLocked NOTIFY changed)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY changed)
    Q_PROPERTY(bool moveToTrashAvailable READ moveToTrashAvailable CONSTANT)
    Q_PROPERTY(bool moveToTrash READ moveToTrash WRITE setMoveToTrash NOTIFY changed)
    Q_PROPERTY(bool syncHiddenFiles READ syncHiddenFiles WRITE setSyncHiddenFiles NOTIFY changed)
    Q_PROPERTY(bool meteredAvailable READ meteredAvailable CONSTANT)
    Q_PROPERTY(bool pauseWhenMetered READ pauseWhenMetered WRITE setPauseWhenMetered NOTIFY changed)
    // ограничение скорости: 0 — без ограничений, -1 — автоматически, 1 — не больше limit КБ/с
    Q_PROPERTY(int downloadMode READ downloadMode WRITE setDownloadMode NOTIFY changed)
    Q_PROPERTY(int downloadLimit READ downloadLimit WRITE setDownloadLimit NOTIFY changed)
    Q_PROPERTY(int uploadMode READ uploadMode WRITE setUploadMode NOTIFY changed)
    Q_PROPERTY(int uploadLimit READ uploadLimit WRITE setUploadLimit NOTIFY changed)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")
    OC_DECLARE_WIDGET_FOCUS

public:
    explicit RagradSettingsPage(QWidget *parent = nullptr);

    bool autostartAvailable() const;
    bool autostartLocked() const;
    bool autostart() const;
    void setAutostart(bool enable);

    bool moveToTrashAvailable() const;
    bool moveToTrash() const;
    void setMoveToTrash(bool enable);

    bool syncHiddenFiles() const;
    void setSyncHiddenFiles(bool enable);

    bool meteredAvailable() const;
    bool pauseWhenMetered() const;
    void setPauseWhenMetered(bool enable);

    int downloadMode() const;
    void setDownloadMode(int mode);
    int downloadLimit() const;
    void setDownloadLimit(int limit);
    int uploadMode() const;
    void setUploadMode(int mode);
    int uploadLimit() const;
    void setUploadLimit(int limit);

    // какие файлы не синхронизируются (окно RagradIgnoreList, ragradservicedialogs.h)
    Q_INVOKABLE void openIgnoredFiles();
    // журнал работы — для поддержки (окно RagradLogSettings)
    Q_INVOKABLE void openLog();

Q_SIGNALS:
    void changed();
    void syncOptionsChanged();

protected:
    void showEvent(QShowEvent *event) override;

private:
    QmlUtils::OCQuickWidget *_quickWidget;
    QPointer<QDialog> _ignoreEditor;
    QPointer<QDialog> _logDialog;
};
}
