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

#pragma once

#include "abstractsetupwizardpage.h"
#include "gui/qmlutils.h"

class QNetworkAccessManager;

namespace OCC {
class QmlAppTokenCredentials;
namespace Wizard {

    class AppTokenCredentialsSetupWizardPage : public AbstractSetupWizardPage
    {
        Q_OBJECT
        OC_DECLARE_WIDGET_FOCUS

    public:
        AppTokenCredentialsSetupWizardPage(QNetworkAccessManager *nam, const QUrl &serverUrl, QWidget *parent = nullptr);

        bool validateInput() const override;

    Q_SIGNALS:
        void loggedIn(const QString &userName, const QString &token, const QString &label);
    };

}
}
