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

#include "creds/apptokenlogin.h"

#include "common/utility.h"
#include "libsync/creds/httpcredentials.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSysInfo>
#include <QUrlQuery>

using namespace Qt::Literals::StringLiterals;

namespace OCC {

Q_LOGGING_CATEGORY(lcAppTokenLogin, "gui.credentials.apptoken.login", QtInfoMsg)

namespace {
    // срок ключа — как в мобильном приложении (lib/config.dart kTokenExpiry)
    const auto tokenExpiryC = u"8760h"_s;

    QString networkErrorText()
    {
        return QObject::tr("Нет связи с сервером. Проверьте интернет и попробуйте ещё раз.");
    }

    QString serverErrorText()
    {
        return QObject::tr("Сервер временно недоступен. Попробуйте позже.");
    }

    int httpStatus(QNetworkReply *reply)
    {
        return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
}

AppTokenLogin::AppTokenLogin(QNetworkAccessManager *nam, const QUrl &serverUrl, QObject *parent)
    : QObject(parent)
    , _nam(nam)
    , _serverUrl(serverUrl)
{
}

QString AppTokenLogin::makeLabel()
{
    return tr("Диск для Windows, %1, вход %2").arg(QSysInfo::machineHostName(), QDateTime::currentDateTime().toString(u"dd.MM.yyyy HH:mm"_s));
}

QNetworkReply *AppTokenLogin::post(const QString &path, const QByteArray &body, const QByteArray &authorization)
{
    QUrl url = Utility::concatUrlPath(_serverUrl, path);
    QNetworkRequest req(url);
    // запросы входа подписываем сами; учётные данные аккаунта (если nam — его) не подмешивать
    req.setAttribute(HttpCredentials::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    req.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_s);
    if (!authorization.isEmpty()) {
        req.setRawHeader("Authorization", authorization);
    }
    return _nam->post(req, body);
}

void AppTokenLogin::start(const QString &login, const QString &password, bool resolve)
{
    const QString trimmed = login.trimmed();
    if (trimmed.isEmpty() || password.isEmpty()) {
        Q_EMIT failed(tr("Введите логин и пароль."));
        return;
    }
    if (!_nam) {
        Q_EMIT failed(networkErrorText());
        return;
    }
    if (!resolve) {
        createToken(trimmed, password);
        return;
    }

    const QByteArray body = QJsonDocument(QJsonObject{{u"login"_s, trimmed}}).toJson(QJsonDocument::Compact);
    auto *reply = post(u"/disk-auth/resolve-login"_s, body);
    connect(reply, &QNetworkReply::finished, this, [reply, trimmed, password, this] {
        reply->deleteLater();
        const int status = httpStatus(reply);
        const QByteArray data = reply->readAll();
        if (status == 0) {
            qCWarning(lcAppTokenLogin) << u"resolve-login: network error" << reply->errorString();
            Q_EMIT failed(networkErrorText());
            return;
        }
        if (status == 200) {
            const QString userName = QJsonDocument::fromJson(data).object().value(u"username"_s).toString();
            if (!userName.isEmpty()) {
                createToken(userName, password);
                return;
            }
        }
        if (status == 404) {
            // внутренний логин можно ввести и напрямую (как в мобильном приложении)
            static const QRegularExpression internalLogin(u"^[a-z][a-z0-9_.-]{1,40}$"_s);
            if (internalLogin.match(trimmed).hasMatch()) {
                createToken(trimmed, password);
                return;
            }
            Q_EMIT failed(tr("Для этого логина нет доступа к Диску. Обратитесь к администратору."));
            return;
        }
        if (status == 429) {
            Q_EMIT failed(tr("Слишком много попыток входа. Подождите минуту."));
            return;
        }
        qCWarning(lcAppTokenLogin) << u"resolve-login: unexpected status" << status;
        Q_EMIT failed(serverErrorText());
    });
}

void AppTokenLogin::createToken(const QString &userName, const QString &password)
{
    const QString label = makeLabel();
    QUrlQuery query;
    query.addQueryItem(u"expiry"_s, tokenExpiryC);
    query.addQueryItem(u"label"_s, label);
    const QByteArray authorization = "Basic " + QStringLiteral("%1:%2").arg(userName, password).toUtf8().toBase64();

    QUrl url = Utility::concatUrlPath(_serverUrl, u"/auth-app/tokens"_s, query);
    QNetworkRequest req(url);
    req.setAttribute(HttpCredentials::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    req.setRawHeader("Authorization", authorization);
    auto *reply = _nam->post(req, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [reply, userName, label, this] {
        reply->deleteLater();
        const int status = httpStatus(reply);
        const QByteArray data = reply->readAll();
        if (status == 0) {
            qCWarning(lcAppTokenLogin) << u"auth-app/tokens: network error" << reply->errorString();
            Q_EMIT failed(networkErrorText());
            return;
        }
        if (status == 401 || status == 403) {
            Q_EMIT failed(tr("Неверный логин или пароль."));
            return;
        }
        const QString token = QJsonDocument::fromJson(data).object().value(u"token"_s).toString();
        if (status != 200 || token.isEmpty()) {
            qCWarning(lcAppTokenLogin) << u"auth-app/tokens: unexpected status" << status;
            Q_EMIT failed(tr("Не удалось войти. Попробуйте позже."));
            return;
        }
        qCInfo(lcAppTokenLogin) << u"device key issued for" << userName;
        Q_EMIT succeeded(userName, token, label);
    });
}

} // namespace OCC
