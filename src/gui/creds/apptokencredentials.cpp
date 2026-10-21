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

#include "creds/apptokencredentials.h"

#include "account.h"
#include "accountmodalwidget.h"
#include "application.h"
#include "common/utility.h"
#include "creds/qmlcredentials.h"
#include "gui/accountsettings.h"
#include "libsync/creds/credentialmanager.h"
#include "libsync/creds/httpcredentials.h"
#include "settingsdialog.h"

#include <QDateTime>
#include <QJsonArray>
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

Q_LOGGING_CATEGORY(lcAppTokenCredentials, "gui.credentials.apptoken", QtInfoMsg)

namespace {
    // группа и ключ в связке ключей (CredentialManager::clear(group) удаляет всю группу)
    const auto keyGroupC = u"ragrad"_s;
    const auto keyC = u"ragrad/apptoken"_s;

    QByteArray basicHeader(const QString &userName, const QString &secret)
    {
        return "Basic " + QStringLiteral("%1:%2").arg(userName, secret).toUtf8().toBase64();
    }

    // тихое продление: срок нового ключа — как при входе (AppTokenLogin), выпускать за 30 дней до конца,
    // проверять через 2 минуты после запуска и потом раз в 12 часов
    const auto renewExpiryC = u"8760h"_s;
    constexpr qint64 renewBeforeDaysC = 30;
    constexpr auto renewFirstCheckC = std::chrono::minutes(2);
    constexpr auto renewIntervalC = std::chrono::hours(12);

    int httpStatus(QNetworkReply *reply)
    {
        return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    /// срок ключа из списка /auth-app/tokens («2026-10-20T19:16:29.081452279Z»); дробную часть Qt не разбирает
    QDateTime parseExpiry(QString value)
    {
        value.remove(QRegularExpression(u"\\.\\d+"_s));
        return QDateTime::fromString(value, Qt::ISODate);
    }
}

class AppTokenAccessManager : public AccessManager
{
public:
    AppTokenAccessManager(const AppTokenCredentials *cred, QObject *parent = nullptr)
        : AccessManager(parent)
        , _cred(cred)
    {
    }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData) override
    {
        QNetworkRequest req(request);
        if (!req.attribute(HttpCredentials::DontAddCredentialsAttribute).toBool()) {
            if (_cred && _cred->_ready) {
                req.setRawHeader("Authorization", basicHeader(_cred->_userName, _cred->_token));
            }
        }
        return AccessManager::createRequest(op, req, outgoingData);
    }

private:
    // учётные данные живут вместе с аккаунтом, а менеджер может пережить их
    QPointer<const AppTokenCredentials> _cred;
};

AppTokenCredentials::AppTokenCredentials(const QString &userName, const QString &token, const QString &label)
    : _userName(userName)
    , _token(token)
    , _label(label)
    , _ready(!userName.isEmpty() && !token.isEmpty())
{
}

AccessManager *AppTokenCredentials::createAM() const
{
    return new AppTokenAccessManager(this);
}

bool AppTokenCredentials::ready() const
{
    return _ready;
}

void AppTokenCredentials::fetchFromKeychain()
{
    _wasFetched = true;
    if (_ready) {
        scheduleRenewal();
        Q_EMIT fetched();
        return;
    }
    auto *job = _account->credentialManager()->get(keyC);
    connect(job, &CredentialJob::finished, this, [job, this] {
        if (job->error() == QKeychain::NoError) {
            const auto data = job->data().toMap();
            _userName = data.value(u"user"_s).toString();
            _token = data.value(u"token"_s).toString();
            _label = data.value(u"label"_s).toString();
            _ready = !_userName.isEmpty() && !_token.isEmpty();
        } else {
            qCWarning(lcAppTokenCredentials) << u"device key not found in keychain:" << job->errorString();
            _ready = false;
        }
        if (_ready) {
            scheduleRenewal();
        }
        Q_EMIT fetched();
    });
}

void AppTokenCredentials::restartOauth()
{
    // ключ не принят (отозван, истёк) или его нет — спросить пароль заново
    if (_modalWidget) {
        return;
    }
    qCInfo(lcAppTokenCredentials) << u"asking the user to log in again";
    auto *qmlCredentials = new QmlAppTokenCredentials(_account->accessManager(), _account->url(), _account->davDisplayName(), _userName);
    _modalWidget = new AccountModalWidget(
        tr("Требуется вход"), QUrl(u"qrc:/qt/qml/eu/OpenCloud/gui/qml/credentials/AppTokenCredentials.qml"_s), qmlCredentials, ocApp()->settingsDialog());

    connect(qmlCredentials, &QmlCredentials::logOutRequested, _modalWidget, [this] {
        _modalWidget->reject();
        _modalWidget.clear();
        Q_EMIT requestLogout();
    });
    connect(qmlCredentials, &QmlAppTokenCredentials::loggedIn, this, [this](const QString &userName, const QString &token, const QString &label) {
        _userName = userName;
        _token = token;
        _label = label;
        _ready = true;
        persist();
        scheduleRenewal();
        if (_modalWidget) {
            _modalWidget->accept();
            _modalWidget.clear();
        }
        Q_EMIT fetched();
    });

    ocApp()->settingsDialog()->accountSettings(_account)->addModalWidget(_modalWidget);
}

void AppTokenCredentials::persist()
{
    if (_userName.isEmpty() || _token.isEmpty()) {
        return;
    }
    _account->credentialManager()->set(keyC, QVariantMap{{u"user"_s, _userName}, {u"token"_s, _token}, {u"label"_s, _label}});
}

void AppTokenCredentials::invalidateToken()
{
    // сервер ключ не принял: обновить его нечем, держать в связке ключей незачем.
    // Логин остаётся в памяти — окно повторного входа спросит только пароль.
    qCWarning(lcAppTokenCredentials) << u"device key rejected, clearing it";
    _token.clear();
    _ready = false;
    _account->clearCookieJar();
    _account->credentialManager()->clear(keyGroupC);
}

void AppTokenCredentials::forgetSensitiveData()
{
    // выход или удаление аккаунта: отозвать ключ на сервере, чтобы им нельзя было войти
    if (!_userName.isEmpty() && !_token.isEmpty()) {
        revokeOnServer(_userName, _token, _label);
    }
    _token.clear();
    _ready = false;
    _account->credentialManager()->clear(keyGroupC);
}

void AppTokenCredentials::revokeOnServer(const QString &userName, const QString &token, const QString &label)
{
    if (label.isEmpty()) {
        qCWarning(lcAppTokenCredentials) << u"device key has no label, cannot revoke it";
        return;
    }
    // отдельный менеджер на приложении: аккаунт при удалении уничтожается раньше, чем дойдут запросы.
    // Сертификаты, которым пользователь доверился для этого аккаунта, переносим — иначе запрос не пройдёт TLS.
    auto *nam = new AccessManager(qApp);
    nam->addCustomTrustedCaCertificates(_account->approvedCerts().values());
    const QUrl serverUrl = _account->url();
    const QByteArray authorization = basicHeader(userName, token);

    QNetworkRequest listReq(Utility::concatUrlPath(serverUrl, u"/auth-app/tokens"_s));
    listReq.setRawHeader("Authorization", authorization);
    auto *listReply = nam->get(listReq);
    QObject::connect(listReply, &QNetworkReply::finished, nam, [nam, listReply, serverUrl, authorization, label] {
        listReply->deleteLater();
        const auto list = QJsonDocument::fromJson(listReply->readAll()).array();
        QString tokenId;
        for (const auto &entry : list) {
            const auto obj = entry.toObject();
            if (obj.value(u"label"_s).toString() == label) {
                tokenId = obj.value(u"token"_s).toString();
                break;
            }
        }
        if (tokenId.isEmpty()) {
            qCWarning(lcAppTokenCredentials) << u"device key to revoke not found on server, status"
                                             << listReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            nam->deleteLater();
            return;
        }
        QUrlQuery query;
        query.addQueryItem(u"token"_s, tokenId);
        QNetworkRequest delReq(Utility::concatUrlPath(serverUrl, u"/auth-app/tokens"_s, query));
        delReq.setRawHeader("Authorization", authorization);
        auto *delReply = nam->deleteResource(delReq);
        QObject::connect(delReply, &QNetworkReply::finished, nam, [nam, delReply] {
            delReply->deleteLater();
            qCInfo(lcAppTokenCredentials) << u"device key revoke, status" << delReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            nam->deleteLater();
        });
    });
}

void AppTokenCredentials::scheduleRenewal()
{
    if (_renewTimer) {
        return;
    }
    _renewTimer = new QTimer(this);
    _renewTimer->setInterval(renewIntervalC);
    connect(_renewTimer, &QTimer::timeout, this, &AppTokenCredentials::checkRenewal);
    _renewTimer->start();
    QTimer::singleShot(renewFirstCheckC, this, &AppTokenCredentials::checkRenewal);
}

void AppTokenCredentials::checkRenewal()
{
    if (!_ready || _renewing || !_account) {
        return;
    }
    if (_label.isEmpty()) {
        // без метки свой ключ в списке не найти; новый с меткой появится при следующем входе паролем
        qCWarning(lcAppTokenCredentials) << u"device key has no label, cannot renew it";
        return;
    }
    _renewing = true;
    const QString token = _token;
    auto *reply = _account->accessManager()->get(QNetworkRequest(Utility::concatUrlPath(_account->url(), u"/auth-app/tokens"_s)));
    connect(reply, &QNetworkReply::finished, this, [reply, token, this] {
        reply->deleteLater();
        // пока шёл запрос, ключ сменили (повторный вход) — у нового свой срок
        if (_token != token) {
            _renewing = false;
            return;
        }
        if (httpStatus(reply) != 200) {
            qCWarning(lcAppTokenCredentials) << u"device key renewal: cannot list keys, status" << httpStatus(reply) << reply->errorString();
            _renewing = false;
            return;
        }
        QDateTime expiry;
        bool found = false;
        const auto list = QJsonDocument::fromJson(reply->readAll()).array();
        for (const auto &entry : list) {
            const auto obj = entry.toObject();
            if (obj.value(u"label"_s).toString() == _label) {
                expiry = parseExpiry(obj.value(u"expiration_date"_s).toString());
                found = true;
                break;
            }
        }
        // ключ не найден или у него нет срока (нулевая дата) — продлевать нечего
        if (!found || !expiry.isValid() || expiry.date().year() < 2000) {
            if (!found) {
                qCWarning(lcAppTokenCredentials) << u"device key renewal: current key not found in the list";
            }
            _renewing = false;
            return;
        }
        const qint64 daysLeft = QDateTime::currentDateTimeUtc().daysTo(expiry);
        if (daysLeft > renewBeforeDaysC) {
            qCInfo(lcAppTokenCredentials) << u"device key is valid for" << daysLeft << u"days";
            _renewing = false;
            return;
        }
        qCInfo(lcAppTokenCredentials) << u"device key expires in" << daysLeft << u"days, renewing";
        createRenewedToken();
    });
}

void AppTokenCredentials::createRenewedToken()
{
    const QString newLabel =
        tr("Диск для Windows, %1, продлён %2").arg(QSysInfo::machineHostName(), QDateTime::currentDateTime().toString(u"dd.MM.yyyy HH:mm"_s));
    QUrlQuery query;
    query.addQueryItem(u"expiry"_s, renewExpiryC);
    query.addQueryItem(u"label"_s, newLabel);
    // новый ключ выпускается от имени действующего: запрос подписывает менеджер аккаунта
    auto *reply = _account->accessManager()->post(QNetworkRequest(Utility::concatUrlPath(_account->url(), u"/auth-app/tokens"_s, query)), QByteArray());
    const QString token = _token;
    connect(reply, &QNetworkReply::finished, this, [reply, token, newLabel, this] {
        reply->deleteLater();
        const int status = httpStatus(reply);
        const QString newToken = (status == 200 || status == 201) ? QJsonDocument::fromJson(reply->readAll()).object().value(u"token"_s).toString() : QString();
        if (newToken.isEmpty()) {
            qCWarning(lcAppTokenCredentials) << u"device key renewal: new key not issued, status" << status << reply->errorString();
            _renewing = false;
            return;
        }
        if (_token != token) {
            // пока выпускали, ключ сменили повторным входом — новый не нужен
            revokeOnServer(_userName, newToken, newLabel);
            _renewing = false;
            return;
        }
        // убедиться, что сервер принимает новый ключ, и только потом заменять действующий
        QNetworkRequest check(Utility::concatUrlPath(_account->url(), u"/graph/v1.0/me"_s));
        check.setAttribute(HttpCredentials::DontAddCredentialsAttribute, true);
        check.setRawHeader("Authorization", basicHeader(_userName, newToken));
        auto *checkReply = _account->accessManager()->get(check);
        connect(checkReply, &QNetworkReply::finished, this, [checkReply, token, newToken, newLabel, this] {
            checkReply->deleteLater();
            if (_token != token) {
                // за время проверки вышли из аккаунта или вошли заново — продлённый ключ не нужен
                revokeOnServer(_userName, newToken, newLabel);
                _renewing = false;
                return;
            }
            if (httpStatus(checkReply) != 200) {
                qCWarning(lcAppTokenCredentials) << u"device key renewal: new key not accepted, status" << httpStatus(checkReply) << u"keeping the current one";
                revokeOnServer(_userName, token, newLabel);
                _renewing = false;
                return;
            }
            finishRenewal(newToken, newLabel);
        });
    });
}

void AppTokenCredentials::finishRenewal(const QString &newToken, const QString &newLabel)
{
    // сначала записать новый ключ в связку ключей и только после успешной записи удалить старый на сервере,
    // иначе после перезапуска клиент мог бы остаться с уже удалённым ключом
    const QString oldToken = _token;
    const QString oldLabel = _label;
    auto *job = _account->credentialManager()->set(keyC, QVariantMap{{u"user"_s, _userName}, {u"token"_s, newToken}, {u"label"_s, newLabel}});
    connect(job, &QKeychain::Job::finished, this, [job, oldToken, oldLabel, newToken, newLabel, this] {
        _renewing = false;
        if (job->error() != QKeychain::NoError) {
            qCWarning(lcAppTokenCredentials) << u"device key renewal: cannot save the new key, keeping the current one:" << job->errorString();
            revokeOnServer(_userName, oldToken, newLabel);
            return;
        }
        if (_token != oldToken) {
            if (_token.isEmpty()) {
                // во время записи вышли из аккаунта: продлённый ключ убрать и из связки ключей, и с сервера
                _account->credentialManager()->clear(keyGroupC);
                revokeOnServer(_userName, newToken, newLabel);
            } else {
                // во время записи прошёл повторный вход: вернуть в связку ключей его ключ, продлённый не нужен
                persist();
                revokeOnServer(_userName, _token, newLabel);
            }
            return;
        }
        _token = newToken;
        _label = newLabel;
        qCInfo(lcAppTokenCredentials) << u"device key renewed";
        revokeOnServer(_userName, newToken, oldLabel);
    });
}

} // namespace OCC
