/*
 * Диск Ра Град: своё автообновление (как в Android-приложении).
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

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;
class QNetworkAccessManager;

namespace OCC {

/**
 * Раз в несколько часов читает файл версии на сервере Диска. Есть версия новее — скачивает установщик
 * в папку временных файлов, сверяет SHA-256 и показывает в окне «Доступна новая версия — Обновить».
 * «Обновить» — программа закрывается и запускает установщик.
 */
class RagradUpdater : public QObject
{
    Q_OBJECT
    // номер скачанной и проверенной новой версии («4.0.5»); пусто — обновлений нет
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY changed)
    QML_ELEMENT
    QML_SINGLETON

public:
    static RagradUpdater *instance();
    static RagradUpdater *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QString availableVersion() const { return _version; }

    /// начать проверки (первая — через пару минут после запуска, дальше раз в 4 часа)
    void start();

    /// закрыть программу и запустить скачанный установщик
    Q_INVOKABLE void installNow();

Q_SIGNALS:
    void changed();

private:
    explicit RagradUpdater(QObject *parent = nullptr);

    void check();
    void download(const QString &version, const QUrl &url, const QByteArray &sha256);
    void setReady(const QString &version, const QString &file);

    QNetworkAccessManager *_nam = nullptr;
    QTimer _timer;
    QString _version;
    QString _file;
    bool _busy = false;
};

} // namespace OCC
