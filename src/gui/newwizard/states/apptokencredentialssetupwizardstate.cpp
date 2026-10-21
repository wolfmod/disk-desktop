/*
 * Диск Ра Град: шаг мастера «логин + пароль -> ключ устройства» вместо OAuth.
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

#include "gui/newwizard/states/apptokencredentialssetupwizardstate.h"
#include "gui/newwizard/pages/apptokencredentialssetupwizardpage.h"

namespace OCC::Wizard {

AppTokenCredentialsSetupWizardState::AppTokenCredentialsSetupWizardState(SetupWizardContext *context)
    : AbstractSetupWizardState(context)
{
    // переход дальше — по успешному входу на странице, кнопка «Далее» не нужна
    _context->window()->disableNextButton();

    auto *page = new AppTokenCredentialsSetupWizardPage(_context->accessManager(), _context->accountBuilder().serverUrl());
    _page = page;

    connect(page, &AppTokenCredentialsSetupWizardPage::loggedIn, this, [this](const QString &userName, const QString &token, const QString &label) {
        _context->window()->slotStartTransition();
        _context->accountBuilder().setAuthenticationStrategy(std::make_unique<AppTokenAuthenticationStrategy>(userName, token, label));
        Q_EMIT evaluationSuccessful();
    });
}

SetupWizardState AppTokenCredentialsSetupWizardState::state() const
{
    return SetupWizardState::CredentialsState;
}

void AppTokenCredentialsSetupWizardState::evaluatePage()
{
    // кнопка «Далее» на этом шаге выключена, вход запускается со страницы
}

} // OCC::Wizard
