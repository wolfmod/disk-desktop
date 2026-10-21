/*
 * Диск Ра Град: учётные данные «логин + ключ устройства» (Basic), вместо OAuth.
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

#include "libsync/creds/abstractcredentials.h"

#include <QPointer>
#include <QTimer>

namespace OCC {

class AccountModalWidget;

/**
 * Каждый запрос подписывается заголовком Basic логин:ключ (так же ходит мобильное приложение
 * и апстримный opencloudcmd, см. src/cmd/tokencredentials.cpp).
 * Логин, ключ и его метка хранятся в связке ключей Windows (CredentialManager, ключ ragrad/apptoken).
 * Ключ выдаётся на год; за 30 дней до конца срока клиент сам выпускает новый, входя действующим
 * (checkRenewal). Если сервер ключ не принял (отозван, сотрудник отключён) — окно повторного входа (restartOauth).
 */
class AppTokenCredentials : public AbstractCredentials
{
    Q_OBJECT
    friend class AppTokenAccessManager;

public:
    AppTokenCredentials() = default;
    AppTokenCredentials(const QString &userName, const QString &token, const QString &label);

    AccessManager *createAM() const override;
    bool ready() const override;
    void fetchFromKeychain() override;
    /// повторный вход: окно с паролем (имя метода — из апстрима)
    void restartOauth() override;
    void persist() override;
    void invalidateToken() override;
    void forgetSensitiveData() override;

    QString userName() const { return _userName; }

private:
    void revokeOnServer(const QString &userName, const QString &token, const QString &label);
    void scheduleRenewal();
    void checkRenewal();
    void createRenewedToken();
    void finishRenewal(const QString &newToken, const QString &newLabel);

    QString _userName;
    QString _token;
    QString _label;
    bool _ready = false;
    bool _renewing = false;

    QPointer<AccountModalWidget> _modalWidget;
    QPointer<QTimer> _renewTimer;
};

} // namespace OCC
