/*
 * Copyright (C) by Cédric Bellegarde <gnumdk@gmail.com>
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

#include "libsync/accountfwd.h"
#include "theme.h"

#include <QHash>
#include <QPointer>
#include <QSystemTrayIcon>

class QAction;
class QIcon;

namespace OCC {
class Folder;
class ProgressInfo;
/**
 * @brief The Systray class
 * @ingroup gui
 */
class Systray : public QSystemTrayIcon
{
    Q_OBJECT

public:
    Systray(QObject *parent = nullptr);

    void setToolTip(const QString &tip);


private:
    void slotTrayMessageIfServerUnsupported(Account *account);

    void slotComputeOverallSyncStatus();

    void computeContextMenu();

    // Диск Ра Град: строка состояния в меню (как у Яндекс.Диска) — «Синхронизировано» / «Синхронизация: 37%» и скорость;
    // тексты — из RagradSyncSummary (тот же источник, что у главного окна)
    void updateStatusActions();

    QPointer<QAction> _statusAction;
    QPointer<QAction> _speedAction;
};

} // namespace OCC
