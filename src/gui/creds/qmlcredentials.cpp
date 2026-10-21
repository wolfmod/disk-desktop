/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "qmlcredentials.h"

#include "creds/apptokenlogin.h"
#include "libsync/theme.h"

#include <QClipboard>
#include <QGuiApplication>

using namespace OCC;


QmlCredentials::QmlCredentials(const QUrl &host, const QString &displayName, QObject *parent)
    : QObject(parent)
    , _host(host)
    , _displayName(displayName)
{
}

bool QmlCredentials::isRefresh() const
{
    return _isRefresh;
}

void QmlCredentials::setIsRefresh(bool newIsRefresh)
{
    if (_isRefresh != newIsRefresh) {
        _isRefresh = newIsRefresh;
        Q_EMIT isRefreshChanged();
    }
}

QmlOAuthCredentials::QmlOAuthCredentials(OAuth *oauth, const QUrl &host, const QString &displayName, QObject *parent)
    : QmlCredentials(host, displayName, parent)
    , _oauth(oauth)
{
    if (_oauth) {
        connect(_oauth, &OAuth::authorisationLinkChanged, this, [this] {
            _ready = true;
            Q_EMIT readyChanged();
        });
        connect(_oauth, &QObject::destroyed, this, &QmlOAuthCredentials::readyChanged);
    }
}

void QmlOAuthCredentials::copyAuthenticationUrlToClipboard()
{
    qApp->clipboard()->setText(_oauth->authorisationLink().toString(QUrl::FullyEncoded));
}

void QmlOAuthCredentials::openAuthenticationUrlInBrowser()
{
    _oauth->openBrowser();
}

bool QmlOAuthCredentials::isReady() const
{
    return isValid() && _ready;
}

bool QmlOAuthCredentials::isValid() const
{
    return _oauth;
}

QmlAppTokenCredentials::QmlAppTokenCredentials(
    QNetworkAccessManager *nam, const QUrl &host, const QString &displayName, const QString &userName, QObject *parent)
    : QmlCredentials(host, displayName, parent)
    , _login(new AppTokenLogin(nam, host, this))
    , _userName(userName)
{
    connect(_login, &AppTokenLogin::succeeded, this, [this](const QString &user, const QString &token, const QString &label) {
        setBusy(false);
        Q_EMIT loggedIn(user, token, label);
    });
    connect(_login, &AppTokenLogin::failed, this, [this](const QString &message) {
        setBusy(false);
        setErrorMessage(message);
    });
}

void QmlAppTokenCredentials::login(const QString &login, const QString &password)
{
    if (_busy) {
        return;
    }
    setErrorMessage({});
    setBusy(true);
    // при повторном входе логин уже внутренний — шаг «телефон/почта -> логин» не нужен
    if (_userName.isEmpty()) {
        _login->start(login, password, true);
    } else {
        _login->start(_userName, password, false);
    }
}

bool QmlAppTokenCredentials::isReady() const
{
    return !_busy;
}

QString QmlAppTokenCredentials::errorMessage() const
{
    return _errorMessage;
}

bool QmlAppTokenCredentials::busy() const
{
    return _busy;
}

void QmlAppTokenCredentials::setBusy(bool busy)
{
    if (_busy != busy) {
        _busy = busy;
        Q_EMIT busyChanged();
        Q_EMIT readyChanged();
    }
}

void QmlAppTokenCredentials::setErrorMessage(const QString &message)
{
    if (_errorMessage != message) {
        _errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}
