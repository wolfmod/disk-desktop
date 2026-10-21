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

#pragma once

#include "gui/newwizard/states/abstractsetupwizardstate.h"

namespace OCC::Wizard {

class AppTokenCredentialsSetupWizardState : public AbstractSetupWizardState
{
    Q_OBJECT

public:
    explicit AppTokenCredentialsSetupWizardState(SetupWizardContext *context);

    SetupWizardState state() const override;

    void evaluatePage() override;
};

} // OCC::Wizard
