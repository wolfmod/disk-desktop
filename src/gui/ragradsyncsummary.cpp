/*
 * Диск Ра Град: общее состояние синхронизации (как у Яндекс.Диска — одно на всю программу, а не по папкам).
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

#include "gui/ragradsyncsummary.h"

#include "common/utility.h"
#include "gui/accountmanager.h"
#include "gui/accountstate.h"
#include "gui/folder.h"
#include "gui/folderman.h"
#include "gui/scheduling/syncscheduler.h"
#include "libsync/progressdispatcher.h"

#include <QApplication>
#include <QDateTime>
#include <QJSEngine>

#include <algorithm>
#include <cmath>

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

namespace OCC {

RagradSyncSummary *RagradSyncSummary::instance()
{
    static auto *summary = new RagradSyncSummary(qApp);
    return summary;
}

RagradSyncSummary *RagradSyncSummary::create(QQmlEngine *, QJSEngine *)
{
    auto *summary = instance();
    QJSEngine::setObjectOwnership(summary, QJSEngine::CppOwnership);
    return summary;
}

RagradSyncSummary::RagradSyncSummary(QObject *parent)
    : QObject(parent)
{
    // события хода синхронизации приходят очень часто — пересчитываем не чаще двух раз в секунду
    _throttle.setSingleShot(true);
    _throttle.setInterval(500ms);
    connect(&_throttle, &QTimer::timeout, this, &RagradSyncSummary::update);

    connect(ProgressDispatcher::instance(), &ProgressDispatcher::progressInfo, this, &RagradSyncSummary::onProgress);
    connect(FolderMan::instance(), &FolderMan::folderSyncStateChange, this, &RagradSyncSummary::scheduleUpdate);
    connect(FolderMan::instance(), &FolderMan::folderListChanged, this, &RagradSyncSummary::scheduleUpdate);
    connect(FolderMan::instance(), &FolderMan::folderRemoved, this, [this](Folder *folder) {
        _progress.remove(folder);
        scheduleUpdate();
    });
    // общая пауза из меню трея останавливает планировщик, а не папки
    connect(FolderMan::instance()->scheduler(), &SyncScheduler::isRunningChanged, this, &RagradSyncSummary::scheduleUpdate);

    auto watchAccount = [this](const AccountStatePtr &accountState) {
        connect(accountState.data(), &AccountState::stateChanged, this, &RagradSyncSummary::scheduleUpdate);
    };
    for (const auto &accountState : AccountManager::instance()->accounts()) {
        watchAccount(accountState);
    }
    connect(AccountManager::instance(), &AccountManager::accountAdded, this, [this, watchAccount](AccountStatePtr accountState) {
        watchAccount(accountState);
        scheduleUpdate();
    });
    connect(AccountManager::instance(), &AccountManager::accountRemoved, this, &RagradSyncSummary::scheduleUpdate);

    update();
}

void RagradSyncSummary::onProgress(Folder *folder, const ProgressInfo &progress)
{
    auto &p = _progress[folder];
    p.running = progress.status() != ProgressInfo::Done && progress.status() != ProgressInfo::None;
    p.completedSize = progress.completedSize();
    p.totalSize = std::max(p.completedSize, progress.totalSize());
    p.completedFiles = progress.completedFiles();
    p.totalFiles = std::max(progress.currentFile(), progress.totalFiles());
    p.bandwidth = progress.isUpdatingEstimates() ? static_cast<qint64>(progress.totalProgress().estimatedBandwidth) : 0;
    // сверка (обход дерева): объём ещё неизвестен — процент не показываем, только какую папку сверяем
    p.discovery = progress.status() == ProgressInfo::Discovery || progress.status() == ProgressInfo::Reconcile;
    p.discoveryFolder = !progress._currentDiscoveredRemoteFolder.isEmpty() ? progress._currentDiscoveredRemoteFolder : progress._currentDiscoveredLocalFolder;

    // текущий файл — самый большой из передаваемых, иначе последний завершённый
    QString item;
    qint64 biggest = -1;
    for (const auto &current : progress._currentItems) {
        if (current._item._size > biggest) {
            biggest = current._item._size;
            item = current._item.localName();
        }
    }
    if (item.isEmpty()) {
        item = progress._lastCompletedItem.localName();
    }
    p.currentItem = item;

    scheduleUpdate();
}

void RagradSyncSummary::scheduleUpdate()
{
    if (!_throttle.isActive()) {
        _throttle.start();
    }
}

void RagradSyncSummary::update()
{
    const auto &accounts = AccountManager::instance()->accounts();
    const auto &folders = FolderMan::instance()->folders();

    bool allSignedOut = !accounts.isEmpty();
    bool anyDisconnected = false;
    for (const auto &accountState : accounts) {
        if (accountState->isSignedOut()) {
            continue;
        }
        allSignedOut = false;
        if (!accountState->isConnected()) {
            anyDisconnected = true;
        }
    }

    bool allPaused = !folders.isEmpty();
    bool anyRunning = false;
    for (auto *folder : folders) {
        if (!folder->isSyncPaused()) {
            allPaused = false;
        }
        if (folder->isSyncRunning()) {
            anyRunning = true;
        }
    }
    const bool schedulerStopped = !folders.isEmpty() && !FolderMan::instance()->scheduler()->isRunning();

    // сводный ход по всем папкам, где сейчас идёт синхронизация
    qint64 totalSize = 0;
    qint64 completedSize = 0;
    qint64 totalFiles = 0;
    qint64 completedFiles = 0;
    qint64 bandwidth = 0;
    QString item;
    Folder *itemFolder = nullptr;
    bool transferring = false;
    QString checking;
    Folder *checkingFolder = nullptr;
    for (auto it = _progress.cbegin(); it != _progress.cend(); ++it) {
        if (!it->running || !folders.contains(it.key())) {
            continue;
        }
        anyRunning = true;
        if (it->discovery) {
            if (checking.isEmpty() && !it->discoveryFolder.isEmpty()) {
                checking = it->discoveryFolder;
                checkingFolder = it.key();
            }
            continue;
        }
        transferring = true;
        totalSize += it->totalSize;
        completedSize += it->completedSize;
        totalFiles += it->totalFiles;
        completedFiles += it->completedFiles;
        bandwidth += it->bandwidth;
        if (item.isEmpty() && !it->currentItem.isEmpty()) {
            item = it->currentItem;
            itemFolder = it.key();
        }
    }

    State state = State::Waiting;
    QString title;
    QString details;
    QString trayText;
    QString speedText;
    double value = -1;

    if (allSignedOut) {
        state = State::SignedOut;
        title = trayText = tr("Выполните вход");
        details = tr("Войдите в аккаунт, чтобы файлы снова синхронизировались.");
    } else if (anyDisconnected) {
        state = State::Offline;
        title = trayText = tr("Нет соединения с сервером");
        details = tr("Проверьте подключение к интернету. Синхронизация продолжится сама, когда связь появится.");
    } else if (allPaused || schedulerStopped) {
        state = State::Paused;
        title = trayText = tr("Синхронизация приостановлена");
        details = tr("Файлы на компьютере и на Диске сейчас не обновляются.");
    } else if (anyRunning) {
        state = State::Syncing;
        // как в списке папок: к байтам прибавляется по «байту» на файл, чтобы процент шёл и при удалении/переименовании
        if (!transferring) {
            title = trayText = tr("Проверка изменений…");
        } else if (totalSize + totalFiles > 0) {
            const int percent =
                std::clamp(static_cast<int>(std::lround(double(completedSize + completedFiles) * 100.0 / double(totalSize + totalFiles))), 0, 100);
            title = tr("Синхронизация — %1%").arg(percent);
            trayText = tr("Синхронизация: %1%").arg(percent);
            value = percent / 100.0;
        } else {
            title = trayText = tr("Проверка изменений…");
        }
        if (bandwidth > 0) {
            speedText = tr("%1/с").arg(Utility::octetsToString(bandwidth));
        }
        QStringList parts;
        if (!transferring && checkingFolder) {
            parts.append(u"%1: %2"_s.arg(checkingFolder->displayName(), checking.replace(u'/', u" / "_s)));
        } else if (itemFolder && !item.isEmpty()) {
            parts.append(u"%1: %2"_s.arg(itemFolder->displayName(), item.replace(u'/', u" / "_s)));
        }
        if (!speedText.isEmpty()) {
            parts.append(speedText);
        }
        details = parts.isEmpty() ? tr("Сверяем файлы на компьютере и на Диске.") : parts.join(u" · "_s);
    } else if (folders.isEmpty()) {
        state = State::Waiting;
        title = trayText = tr("Подготовка папок…");
        details = tr("Папки Диска появятся на компьютере через несколько секунд.");
    } else {
        switch (FolderMan::trayOverallStatus(folders).overallStatus().status()) {
        case SyncResult::Success:
        case SyncResult::Problem: {
            state = State::Synced;
            title = tr("Всё синхронизировано");
            trayText = tr("Синхронизировано");
            QDateTime last;
            for (auto *folder : folders) {
                const auto time = folder->syncResult().syncTime();
                if (time.isValid() && (!last.isValid() || time > last)) {
                    last = time;
                }
            }
            if (last.isValid()) {
                const auto local = last.toLocalTime();
                details = local.date() == QDate::currentDate()
                    ? tr("Последняя синхронизация сегодня в %1").arg(local.toString(u"HH:mm"_s))
                    : tr("Последняя синхронизация %1 в %2").arg(local.toString(u"dd.MM.yyyy"_s), local.toString(u"HH:mm"_s));
            } else {
                details = tr("Файлы на компьютере совпадают с файлами на Диске.");
            }
            break;
        }
        case SyncResult::Error:
        case SyncResult::SetupError:
            state = State::Error;
            title = tr("Ошибка синхронизации");
            trayText = tr("Ошибка синхронизации — откройте окно программы");
            details = tr("Подробности — у папки ниже.");
            break;
        case SyncResult::Paused:
            state = State::Paused;
            title = trayText = tr("Синхронизация приостановлена");
            details = tr("Файлы на компьютере и на Диске сейчас не обновляются.");
            break;
        case SyncResult::Offline:
            state = State::Offline;
            title = trayText = tr("Нет соединения с сервером");
            details = tr("Проверьте подключение к интернету. Синхронизация продолжится сама, когда связь появится.");
            break;
        default:
            state = State::Waiting;
            title = trayText = tr("Ожидание синхронизации");
            break;
        }
    }

    if (state == _state && title == _title && details == _details && trayText == _trayText && speedText == _speedText && value == _progressValue) {
        return;
    }
    _state = state;
    _title = title;
    _details = details;
    _trayText = trayText;
    _speedText = speedText;
    _progressValue = value;
    Q_EMIT changed();
}

void RagradSyncSummary::resume()
{
    for (auto *folder : FolderMan::instance()->folders()) {
        if (folder->isSyncPaused()) {
            // как AccountSettings::slotEnableCurrentFolder: не забыть локальные изменения за время паузы
            folder->slotNextSyncFullLocalDiscovery();
            folder->setSyncPaused(false);
            FolderMan::instance()->scheduler()->enqueueFolder(folder);
        }
    }
    FolderMan::instance()->scheduler()->start();
    scheduleUpdate();
}

}
