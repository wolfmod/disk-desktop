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

#include "gui/ragradupdater.h"

#include "gui/settingsdialog.h"

#include "gui/application.h"
#include "gui/notifications/systemnotification.h"
#include "gui/notifications/systemnotificationmanager.h"
#include "libsync/common/version.h"
#include "resources/fonticon.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVersionNumber>

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

namespace OCC {

Q_LOGGING_CATEGORY(lcRagradUpdater, "gui.ragrad.updater", QtInfoMsg)

namespace {
    // файл версии рядом с установщиком: {"version":"4.0.5","url":"…/DiskSetup.exe","sha256":"…"}
    const auto versionUrlC = u"https://cloud.ra-grad.ru/app/download/windows-version.json"_s;
    const auto allowedPrefixC = u"https://cloud.ra-grad.ru/"_s;

    QByteArray fileSha256(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            return {};
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&f)) {
            return {};
        }
        return hash.result().toHex();
    }
}

RagradUpdater *RagradUpdater::instance()
{
    static auto *updater = new RagradUpdater(qApp);
    return updater;
}

RagradUpdater *RagradUpdater::create(QQmlEngine *, QJSEngine *)
{
    auto *updater = instance();
    QJSEngine::setObjectOwnership(updater, QJSEngine::CppOwnership);
    return updater;
}

RagradUpdater::RagradUpdater(QObject *parent)
    : QObject(parent)
    , _nam(new QNetworkAccessManager(this))
{
    _timer.setInterval(4h);
    connect(&_timer, &QTimer::timeout, this, &RagradUpdater::check);
}

void RagradUpdater::start()
{
    if (_timer.isActive()) {
        return;
    }
    _timer.start();
    QTimer::singleShot(2min, this, &RagradUpdater::check);
}

void RagradUpdater::check()
{
    if (_busy) {
        return;
    }
    _busy = true;
    QNetworkRequest req{QUrl(versionUrlC)};
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setTransferTimeout(30000);
    auto *reply = _nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        _busy = false;
        if (reply->error() != QNetworkReply::NoError) {
            qCInfo(lcRagradUpdater) << u"файл версии недоступен:" << reply->errorString();
            return;
        }
        const auto json = QJsonDocument::fromJson(reply->readAll()).object();
        const QString version = json.value("version"_L1).toString().trimmed();
        const QUrl url(json.value("url"_L1).toString().trimmed());
        const QByteArray sha256 = json.value("sha256"_L1).toString().trimmed().toLower().toLatin1();
        const auto remote = QVersionNumber::fromString(version);
        if (remote.isNull() || remote <= Version::version()) {
            return; // новее нет
        }
        if (!url.toString().startsWith(allowedPrefixC) || sha256.size() != 64) {
            qCWarning(lcRagradUpdater) << u"файл версии без адреса установщика или контрольной суммы — пропуск";
            return;
        }
        if (version == _version) {
            return; // уже скачана
        }
        download(version, url, sha256);
    });
}

void RagradUpdater::download(const QString &version, const QUrl &url, const QByteArray &sha256)
{
    const QString target = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(u"DiskSetup-%1.exe"_s.arg(version));
    if (QFile::exists(target) && fileSha256(target) == sha256) {
        setReady(version, target);
        return;
    }
    _busy = true;
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *file = new QSaveFile(target, this);
    if (!file->open(QIODevice::WriteOnly)) {
        qCWarning(lcRagradUpdater) << u"не открыть для записи" << target;
        file->deleteLater();
        _busy = false;
        return;
    }
    auto *reply = _nam->get(req);
    connect(reply, &QIODevice::readyRead, this, [reply, file] { file->write(reply->readAll()); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, version, target, sha256] {
        reply->deleteLater();
        file->deleteLater();
        _busy = false;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status != 200) {
            qCWarning(lcRagradUpdater) << u"установщик не скачан:" << status << reply->errorString();
            file->cancelWriting();
            return;
        }
        file->write(reply->readAll());
        if (!file->commit()) {
            qCWarning(lcRagradUpdater) << u"установщик не сохранён" << target;
            return;
        }
        if (fileSha256(target) != sha256) {
            qCWarning(lcRagradUpdater) << u"контрольная сумма установщика не совпала — файл удалён";
            QFile::remove(target);
            return;
        }
        setReady(version, target);
    });
}

void RagradUpdater::setReady(const QString &version, const QString &file)
{
    const bool firstTime = _version != version;
    _version = version;
    _file = file;
    Q_EMIT changed();
    qCInfo(lcRagradUpdater) << u"готово обновление" << version << file;
    if (firstTime) {
        // 4.0.7: в уведомлении Windows — кнопки (уведомление с кнопками висит, пока человек не ответит, как уведомления
        // о разделах с «Отметить прочитанным»). 4.0.8: «Открыть Диск» и щелчок по уведомлению — окно программы
        // на главной странице, где плашка «Доступна новая версия — Обновить»; «Обновить» — сразу установка.
        const QString openButton = tr("Открыть Диск");
        const QString updateButton = tr("Обновить");
        SystemNotificationRequest request(tr("Доступна новая версия Диска %1").arg(version),
            tr("Нажмите «Обновить», чтобы установить её, или «Открыть Диск», чтобы посмотреть в окне программы."), Resources::FontIcon(QChar(0xf021)));
        request.setButtons({updateButton, openButton});
        if (auto *n = ocApp()->systemNotificationManager()->notify(std::move(request))) {
            connect(n, &SystemNotification::buttonClicked, this, [this, updateButton, openButton](const QString &clicked) {
                if (clicked == updateButton) {
                    installNow();
                } else if (clicked == openButton) {
                    showMainWindow();
                }
            });
            connect(n, &SystemNotification::finished, this, [this](SystemNotification::Result result) {
                if (result == SystemNotification::Result::Clicked) {
                    showMainWindow();
                }
            });
        }
    }
}

void RagradUpdater::showMainWindow()
{
    // главная страница окна — там плашка «Доступна новая версия Диска — Обновить»
    ocApp()->settingsDialog()->setCurrentPage(SettingsDialog::SettingsPage::Account);
    ocApp()->showSettings();
}

void RagradUpdater::installNow()
{
    if (_file.isEmpty() || !QFile::exists(_file)) {
        return;
    }
    QString path = QDir::toNativeSeparators(_file);
    path.replace("'"_L1, "''"_L1);
    // программа закрывается, через 2 с запускается установщик (окно установки, как при ручной установке)
    const QString command = u"Start-Sleep -Seconds 2; Start-Process -FilePath '%1'"_s.arg(path);
    const bool ok = QProcess::startDetached(
        u"powershell.exe"_s, {u"-NoProfile"_s, u"-ExecutionPolicy"_s, u"Bypass"_s, u"-WindowStyle"_s, u"Hidden"_s, u"-Command"_s, command});
    qCInfo(lcRagradUpdater) << u"запуск установщика" << _file << ok;
    if (ok) {
        QTimer::singleShot(0, qApp, &QApplication::quit);
    }
}

} // namespace OCC
