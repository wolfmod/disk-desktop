/*
 * Диск Ра Град: вход по логину и паролю с обменом пароля на ключ устройства.
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
#include <QPointer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace OCC {

/**
 * Вход сотрудника, как в мобильном приложении «Диск»:
 * 1. телефон или почта -> внутренний логин s<номер> (POST /disk-auth/resolve-login);
 * 2. пароль -> ключ устройства (POST /auth-app/tokens, срок 8760h, метка с именем компьютера).
 * Пароль нигде не сохраняется; дальше клиент работает только с ключом (Basic логин:ключ).
 * Внутренний логин можно передать сразу (повторный вход) — тогда шаг 1 пропускается.
 */
class AppTokenLogin : public QObject
{
    Q_OBJECT
public:
    AppTokenLogin(QNetworkAccessManager *nam, const QUrl &serverUrl, QObject *parent = nullptr);

    /// login — телефон, почта или внутренний логин; resolve=false — login уже внутренний
    void start(const QString &login, const QString &password, bool resolve = true);

    /// метка ключа: по ней ключ находится и отзывается при выходе
    static QString makeLabel();

Q_SIGNALS:
    void succeeded(const QString &userName, const QString &token, const QString &label);
    void failed(const QString &message);

private:
    void createToken(const QString &userName, const QString &password);
    QNetworkReply *post(const QString &path, const QByteArray &body, const QByteArray &authorization = {});

    QPointer<QNetworkAccessManager> _nam;
    QUrl _serverUrl;
};

} // namespace OCC
