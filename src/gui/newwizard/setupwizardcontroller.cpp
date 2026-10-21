#include "setupwizardcontroller.h"

#include "gui/application.h"
#include "gui/folderman.h"
#include "libsync/globalconfig.h"
#include "pages/accountconfiguredwizardpage.h"
#include "states/abstractsetupwizardstate.h"
#include "states/accountconfiguredsetupwizardstate.h"
#include "states/apptokencredentialssetupwizardstate.h"
#include "states/serverurlsetupwizardstate.h"
#include "theme.h"

#include <QTimer>

using namespace std::chrono_literals;

namespace {

using namespace OCC;
using namespace OCC::Wizard;

using namespace SetupWizardControllerPrivate;

/**
 * Диск Ра Град: адрес сервера зашит в теме (serverUrl). Тогда шаги «адрес сервера» и «настройка синхронизации»
 * проходятся сами (папка «Диск Ра Град», файлы по запросу), видимым остаётся только вход — навигация не нужна.
 */
bool serverUrlIsFixed()
{
    return GlobalConfig::serverUrl().isValid();
}

/**
 * Generate list of wizard states to put in the navigation.
 * The actual wizard may be in states not within this list to perform tasks in the background without user interaction
 * (e.g., to detect the authentication method the server uses).
 */
QList<SetupWizardState> getNavigationEntries()
{
    if (serverUrlIsFixed()) {
        return {};
    }
    return {
        SetupWizardState::ServerUrlState,
        SetupWizardState::CredentialsState,
        SetupWizardState::AccountConfiguredState,
    };
}

}

namespace OCC::Wizard {

Q_LOGGING_CATEGORY(lcSetupWizardController, "gui.setupwizard.controller")

SetupWizardController::SetupWizardController(SettingsDialog *parent)
    : QObject(parent)
    , _context(new SetupWizardContext(parent, this))
{
    _context->window()->setNavigationEntries(getNavigationEntries());

    // we always switch to this state, even if the URL is overridden by the theme
    // it will detect
    changeStateTo(SetupWizardState::FirstState);

    // allow settings dialog to clean up the wizard controller and all the objects it created
    connect(_context->window(), &SetupWizardWidget::rejected, this, [this]() {
        qCDebug(lcSetupWizardController) << u"wizard window closed";
        Q_EMIT finished(nullptr, SyncMode::Invalid);
    });

    connect(_context->window(), &SetupWizardWidget::navigationEntryClicked, this, [this](SetupWizardState clickedState) {
        qCDebug(lcSetupWizardController) << u"pagination entry clicked: current state" << _currentState << u"clicked state" << clickedState;
        changeStateTo(clickedState);
    });

    connect(_context->window(), &SetupWizardWidget::nextButtonClicked, this, [this]() {
        qCDebug(lcSetupWizardController) << u"next button clicked, current state" << _currentState;
        _currentState->evaluatePage();
    });

    // in case the back button is clicked, the current page's data is dismissed, and the previous page should be shown
    connect(_context->window(), &SetupWizardWidget::backButtonClicked, this, [this]() {
        // with enum classes, we have to explicitly cast to a numeric value
        const auto currentStateIdx = static_cast<int>(_currentState->state());
        Q_ASSERT(currentStateIdx > 0);

        qCDebug(lcSetupWizardController) << u"back button clicked, current state" << _currentState;

        auto previousState = static_cast<SetupWizardState>(currentStateIdx - 1);
        changeStateTo(previousState);
    });
}

SetupWizardWidget *SetupWizardController::window()
{
    return _context->window();
}

void SetupWizardController::changeStateTo(SetupWizardState nextState, ChangeReason reason)
{
    // validate initial state
    Q_ASSERT(nextState == SetupWizardState::ServerUrlState || _currentState != nullptr);

    if (_currentState != nullptr) {
        _currentState->deleteLater();
    }

    switch (nextState) {
    case SetupWizardState::ServerUrlState: {
        _currentState = new ServerUrlSetupWizardState(_context);
        break;
    }
    case SetupWizardState::CredentialsState:
        // Диск Ра Град: вместо OAuth — логин и пароль с обменом на ключ устройства
        _currentState = new AppTokenCredentialsSetupWizardState(_context);
        break;
    case SetupWizardState::AccountConfiguredState: {
        _currentState = new AccountConfiguredSetupWizardState(_context);

        switch (reason) {
        case ChangeReason::Default:
            [[fallthrough]];
        case ChangeReason::EvaluationRetry:
            break;
        case ChangeReason::EvaluationFailed:
            // whenever the evaluation of the last page fails, it's safe to assume it's due to some issue with the advanced
            // therefore, we want to show them in that case
            auto *page = dynamic_cast<AccountConfiguredWizardPage *>(_currentState->page());
            if (OC_ENSURE(page != nullptr)) {
                page->setShowAdvancedSettings(true);
            }
        }
        break;
    }
    }

    Q_ASSERT(_currentState != nullptr);
    Q_ASSERT(_currentState->state() == nextState);

    qCDebug(lcSetupWizardController) << u"Current wizard state:" << _currentState->state();

    connect(_currentState, &AbstractSetupWizardState::evaluationSuccessful, this, [this]() {
        switch (_currentState->state()) {
        case SetupWizardState::ServerUrlState: {
            changeStateTo(SetupWizardState::CredentialsState);
            return;
        }
        case SetupWizardState::CredentialsState: {
            // for now, we assume there is only a single instance
            const auto webFingerInstances = _context->accountBuilder().webFingerInstances();
            if (!webFingerInstances.isEmpty()) {
                Q_ASSERT(webFingerInstances.size() == 1);
                _context->accountBuilder().setWebFingerSelectedInstance(webFingerInstances.front());
            }

            // not a fan of performing this job here, should be moved into its own (headless) state IMO
            // we can bind it to the current state, which will be cleaned up by changeStateTo(...) as soon as the job finished
            auto fetchUserInfoJob = _context->startFetchUserInfoJob(_currentState);

            connect(fetchUserInfoJob, &CoreJob::finished, this, [this, fetchUserInfoJob] {
                if (fetchUserInfoJob->success()) {
                    auto result = fetchUserInfoJob->result().value<FetchUserInfoResult>();
                    _context->accountBuilder().setDisplayName(result.displayName());
                    changeStateTo(SetupWizardState::AccountConfiguredState);
                } else if (fetchUserInfoJob->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
                    _context->window()->showErrorMessage(tr("Invalid credentials"));
                    changeStateTo(_currentState->state());
                } else {
                    _context->window()->showErrorMessage(tr("Failed to retrieve user information from server"));
                    changeStateTo(_currentState->state());
                }
            });

            return;
        }
        case SetupWizardState::AccountConfiguredState: {
            const auto *pagePtr = qobject_cast<AccountConfiguredWizardPage *>(_currentState->page());
            auto account = _context->accountBuilder().build();
            Q_ASSERT(account != nullptr);
            Q_EMIT finished(account, pagePtr->syncMode());
            return;
        }
        default:
            Q_UNREACHABLE();
        }
    });

    connect(_currentState, &AbstractSetupWizardState::evaluationFailed, this, [this](const QString &errorMessage) {
        _context->window()->showErrorMessage(errorMessage);
        _currentState->deleteLater();
        changeStateTo(_currentState->state(), ChangeReason::EvaluationFailed);
    });

    connect(_currentState, &AbstractSetupWizardState::evaluationRetry, this, [this] {
        _currentState->deleteLater();
        _context->window()->slotStartTransition();
        changeStateTo(_currentState->state(), ChangeReason::EvaluationRetry);
    });

    _context->window()->displayPage(_currentState->page(), _currentState->state());

    // Диск Ра Град: адрес зашит — проверяем сервер сразу, без нажатия «Далее»; после входа так же сразу
    // завершаем настройку (папка «Диск Ра Град» по умолчанию, файлы по запросу) — без экрана «Тонкая настройка».
    // При ошибке страница остаётся с текстом ошибки (у последнего шага — с открытыми настройками папки), «Далее» повторяет.
    const bool autoState = _currentState->state() == SetupWizardState::ServerUrlState || _currentState->state() == SetupWizardState::AccountConfiguredState;
    if (autoState && serverUrlIsFixed() && reason == ChangeReason::Default) {
        AbstractSetupWizardState *state = _currentState.data();
        QTimer::singleShot(0, state, [this, state] {
            if (_currentState.data() == state) {
                _context->window()->slotStartTransition();
                state->evaluatePage();
            }
        });
    }
}

SetupWizardController::~SetupWizardController() noexcept { }
}
