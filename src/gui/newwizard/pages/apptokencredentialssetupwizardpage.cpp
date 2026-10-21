/*
 * Диск Ра Град: страница мастера «логин + пароль».
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

#include "apptokencredentialssetupwizardpage.h"

#include "gui/creds/qmlcredentials.h"
#include "gui/qmlutils.h"

#include <QHBoxLayout>

namespace OCC::Wizard {

AppTokenCredentialsSetupWizardPage::AppTokenCredentialsSetupWizardPage(QNetworkAccessManager *nam, const QUrl &serverUrl, QWidget *parent)
    : AbstractSetupWizardPage(parent)
{
    auto *layout = new QHBoxLayout(this);
    auto *widget = new QmlUtils::OCQuickWidget;
    layout->addWidget(widget);
    setFocusProxy(widget);

    auto *credentials = new QmlAppTokenCredentials(nam, serverUrl, {});
    credentials->setIsRefresh(false);
    connect(credentials, &QmlAppTokenCredentials::loggedIn, this, &AppTokenCredentialsSetupWizardPage::loggedIn);
    widget->setOCContext(
        QUrl(QStringLiteral("qrc:/qt/qml/eu/OpenCloud/gui/qml/credentials/AppTokenCredentials.qml")), this, credentials, QJSEngine::JavaScriptOwnership);
}

bool AppTokenCredentialsSetupWizardPage::validateInput() const
{
    // вход идёт кнопкой «Войти» на самой странице; дальше мастер переходит сам
    return false;
}

} // namespace OCC::Wizard
