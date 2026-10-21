/*
 * Диск Ра Град: общее состояние синхронизации (как у Яндекс.Диска — одно на всю программу, а не по папкам).
 * Один источник для главного окна (QML-синглтон RagradSyncSummary) и строки состояния в меню трея.
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

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace OCC {
class Folder;
class ProgressInfo;

class RagradSyncSummary : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY changed)
    // заголовок блока состояния в окне: «Всё синхронизировано», «Синхронизация — 37%»…
    Q_PROPERTY(QString title READ title NOTIFY changed)
    // строка под заголовком: текущий файл и скорость, время последней синхронизации, что делать
    Q_PROPERTY(QString details READ details NOTIFY changed)
    // строка состояния в меню трея: «Синхронизировано», «Синхронизация: 37%»…
    Q_PROPERTY(QString trayText READ trayText NOTIFY changed)
    // «3,4 МБ/с» или пусто
    Q_PROPERTY(QString speedText READ speedText NOTIFY changed)
    // 0…1; меньше нуля — объём ещё неизвестен (идёт проверка изменений)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class State { Synced, Syncing, Paused, Offline, SignedOut, Error, Waiting };
    Q_ENUM(State)

    static RagradSyncSummary *instance();
    static RagradSyncSummary *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    State state() const { return _state; }
    QString title() const { return _title; }
    QString details() const { return _details; }
    QString trayText() const { return _trayText; }
    QString speedText() const { return _speedText; }
    double progress() const { return _progressValue; }

    // возобновить всё: и общую паузу (меню трея), и приостановленные папки
    Q_INVOKABLE void resume();

Q_SIGNALS:
    void changed();

private:
    explicit RagradSyncSummary(QObject *parent = nullptr);

    void onProgress(Folder *folder, const ProgressInfo &progress);
    void scheduleUpdate();
    void update();

    struct FolderProgress
    {
        qint64 totalSize = 0;
        qint64 completedSize = 0;
        qint64 totalFiles = 0;
        qint64 completedFiles = 0;
        qint64 bandwidth = 0; // байт/с
        bool running = false;
        bool discovery = false; // идёт сверка (обход дерева), объём ещё неизвестен
        QString currentItem; // путь файла внутри папки
        QString discoveryFolder; // какую подпапку сейчас сверяем
    };

    QHash<Folder *, FolderProgress> _progress;
    QTimer _throttle;

    State _state = State::Waiting;
    QString _title;
    QString _details;
    QString _trayText;
    QString _speedText;
    double _progressValue = -1;
};
}
