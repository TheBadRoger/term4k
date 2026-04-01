#include "UserLoginUI.h"

#include "config/RuntimeConfigs.h"
#include "services/I18nService.h"
#include "utils/ErrorNotifier.h"
#include "utils/tui/InputDecoder.h"

#include <cctype>
#include <iostream>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace ui {

namespace {
class ScopedRawMode {
public:
    explicit ScopedRawMode(const int fd) : fd_(fd) {
        if (fd_ < 0 || !isatty(fd_)) return;
        if (tcgetattr(fd_, &original_) != 0) return;

        termios raw = original_;
        raw.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(fd_, TCSANOW, &raw) == 0) enabled_ = true;
    }

    ~ScopedRawMode() {
        if (enabled_) tcsetattr(fd_, TCSANOW, &original_);
    }

private:
    int fd_ = -1;
    termios original_{};
    bool enabled_ = false;
};

bool pollStdin(const int timeoutMs) {
    pollfd pfd{};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    const int rc = ::poll(&pfd, 1, timeoutMs);
    return rc > 0 && (pfd.revents & POLLIN);
}

std::string readInputChunk() {
    char buffer[128] = {};
    const ssize_t n = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (n <= 0) return "";
    return std::string(buffer, static_cast<std::size_t>(n));
}

std::string tr(const std::string &key) {
    auto &i18n = I18nService::instance();
    i18n.ensureLocaleLoaded(RuntimeConfigs::locale);
    return i18n.get(key);
}

void setErrorAndNotify(std::string &lastError, const std::string &key) {
    lastError = tr(key);
    ErrorNotifier::notify("UserLoginUI", lastError);
}

std::string kvLine(const std::string &labelKey, const std::string &value) {
    return tr(labelKey) + ": " + value;
}
} // namespace

UserLoginUI::UserLoginUI(UserLoginInstance *instance) : instance_(instance) {}

void UserLoginUI::bindInstance(UserLoginInstance *instance) {
    instance_ = instance;
}

void UserLoginUI::show() {
    if (loopCore_.running()) return;
    renderBuffer_ = std::make_unique<tui::RenderBuffer>(100, 8);
    statusLine_.clear();
    lastError_.clear();
    if (usernameInput_.empty() && passwordInput_.empty()) {
        activeField_ = ActiveField::Username;
    }
    blinkCounter_ = 0;
    submitSucceeded_ = false;

    loopCore_.start([this]() {
        static thread_local ScopedRawMode rawMode(STDIN_FILENO);
        loopOnce();
    }, 60);
}

void UserLoginUI::hide() {
    loopCore_.stop();
    renderBuffer_.reset();
    std::cout << "\x1b[0m\x1b[?25h" << std::flush;
}

void UserLoginUI::setUsernameInput(const std::string &username) {
    usernameInput_ = username;
}

void UserLoginUI::setPasswordInput(const std::string &password) {
    passwordInput_ = password;
}

void UserLoginUI::setSubmitModeLogin(const bool loginMode) {
    submitMode_ = loginMode ? SubmitMode::Login : SubmitMode::Register;
}

bool UserLoginUI::submit(std::string *outError) {
    const bool ok = submitCurrentInput();
    if (ok) {
        if (outError != nullptr) outError->clear();
    } else if (outError != nullptr) {
        *outError = lastError_;
    }
    return ok;
}

bool UserLoginUI::consumeSubmitSucceeded() {
    const bool wasSucceeded = submitSucceeded_;
    submitSucceeded_ = false;
    return wasSucceeded;
}

bool UserLoginUI::registerUser(const std::string &username, const std::string &password, std::string *outError) {
    if (instance_ == nullptr) {
        setErrorAndNotify(lastError_, "ui.error.login_instance_unbound");
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (!instance_->registerUser(username, password, &lastError_)) {
        ErrorNotifier::notify("UserLoginUI", lastError_);
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (outError != nullptr) outError->clear();
    lastError_.clear();
    return true;
}

bool UserLoginUI::login(const std::string &username, const std::string &password, std::string *outError) {
    if (instance_ == nullptr) {
        setErrorAndNotify(lastError_, "ui.error.login_instance_unbound");
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (!instance_->login(username, password)) {
        setErrorAndNotify(lastError_, "ui.login.result.login_failed");
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (outError != nullptr) outError->clear();
    lastError_.clear();
    return true;
}

void UserLoginUI::logout() {
    if (instance_ == nullptr) return;
    instance_->logout();
}

bool UserLoginUI::isLoggedIn() const {
    if (instance_ == nullptr) return false;
    return instance_->isLoggedIn();
}

std::optional<User> UserLoginUI::currentUser() const {
    if (instance_ == nullptr) return std::nullopt;
    return instance_->currentUser();
}

const std::string &UserLoginUI::lastError() const {
    return lastError_;
}

void UserLoginUI::loopOnce() {
    ++blinkCounter_;
    renderFrame();

    if (!pollStdin(0)) return;

    const std::string chunk = readInputChunk();
    if (chunk.empty()) return;

    handleInputBytes(chunk);
}

void UserLoginUI::handleInputBytes(const std::string &chunk) {
    const tui::InputDecoder decoder;
    const auto actions = decoder.decode(chunk);

    for (const tui::InputAction action : actions) {
        switch (action) {
            case tui::InputAction::MoveUp:
                activeField_ = ActiveField::Username;
                break;
            case tui::InputAction::MoveDown:
                activeField_ = ActiveField::Password;
                break;
            case tui::InputAction::Confirm:
                submitCurrentInput();
                break;
            case tui::InputAction::Quit:
                loopCore_.requestStop();
                return;
            default:
                break;
        }
    }

    for (char ch : chunk) {
        if (ch == '\t') {
            if (activeField_ == ActiveField::Username) {
                activeField_ = ActiveField::Password;
            } else {
                activeField_ = ActiveField::Username;
            }
            continue;
        }

        if (ch == '\x12') {
            // Ctrl+R toggles between login and register mode.
            submitMode_ = (submitMode_ == SubmitMode::Login) ? SubmitMode::Register : SubmitMode::Login;
            continue;
        }

        if (ch == '\x7f' || ch == '\b') {
            std::string &target = (activeField_ == ActiveField::Username) ? usernameInput_ : passwordInput_;
            if (!target.empty()) target.pop_back();
            continue;
        }

        if (!isPrintableAscii(ch)) continue;

        std::string &target = (activeField_ == ActiveField::Username) ? usernameInput_ : passwordInput_;
        if (target.size() < 48) target.push_back(ch);
    }
}

bool UserLoginUI::submitCurrentInput() {
    submitSucceeded_ = false;

    if (usernameInput_.empty() || passwordInput_.empty()) {
        setErrorAndNotify(lastError_, "ui.login.error.required_fields");
        statusLine_.clear();
        activeField_ = usernameInput_.empty() ? ActiveField::Username : ActiveField::Password;
        return false;
    }

    std::string error;
    bool ok = false;
    if (submitMode_ == SubmitMode::Login) {
        ok = login(usernameInput_, passwordInput_, &error);
        statusLine_ = ok ? tr("ui.login.result.login_succeeded") : tr("ui.login.result.login_failed");
    } else {
        ok = registerUser(usernameInput_, passwordInput_, &error);
        statusLine_ = ok ? tr("ui.login.result.register_succeeded") : tr("ui.login.result.register_failed");
    }

    if (!ok) {
        if (error.empty()) {
            setErrorAndNotify(lastError_, "ui.error.operation_failed");
        } else {
            lastError_ = error;
            ErrorNotifier::notify("UserLoginUI", lastError_);
        }
        activeField_ = ActiveField::Password;
        return false;
    }

    lastError_.clear();
    passwordInput_.clear();
    submitSucceeded_ = true;
    return true;
}

void UserLoginUI::renderFrame() {
    if (!renderBuffer_) return;

    renderBuffer_->clear();
    renderBuffer_->setLine(0, tr("ui.login.title"));
    renderBuffer_->setLine(1, tr("ui.login.hint"));
    renderBuffer_->setLine(2, kvLine("ui.common.scene_pacing", std::to_string(loopCore_.currentFps()) + " " +
                                                              tr("ui.common.fps_unit")));
    renderBuffer_->setLine(3, kvLine("ui.login.mode", submitMode_ == SubmitMode::Login
                                                         ? tr("ui.login.mode.login")
                                                         : tr("ui.login.mode.register")));

    const bool blinkOn = ((blinkCounter_ / 20) % 2) == 0;
    const std::string cursor = blinkOn ? "_" : " ";
    const std::string usernameValue = usernameInput_ + (activeField_ == ActiveField::Username ? cursor : "");
    const std::string passwordValue = maskPassword(passwordInput_) + (activeField_ == ActiveField::Password ? cursor : "");
    renderBuffer_->setLine(4, std::string(activeField_ == ActiveField::Username ? "> " : "  ") + tr("ui.login.username") + ": " +
                                 usernameValue);
    renderBuffer_->setLine(5, std::string(activeField_ == ActiveField::Password ? "> " : "  ") + tr("ui.login.password") + ": " +
                                 passwordValue);

    if (isLoggedIn()) {
        const auto user = currentUser();
        const std::string name = user.has_value() ? user->getUsername() : tr("ui.login.user_unknown");
        renderBuffer_->setLine(6, kvLine("ui.login.status", tr("ui.login.status.logged_in")));
        renderBuffer_->setLine(7, kvLine("ui.login.user", name));
    } else {
        renderBuffer_->setLine(6, kvLine("ui.login.status", tr("ui.login.status.not_logged_in")));
    }

    if (!statusLine_.empty()) renderBuffer_->setLine(7, kvLine("ui.login.result", statusLine_));

    if (!lastError_.empty()) {
        renderBuffer_->setLine(7, kvLine("ui.common.error", lastError_));
    }

    std::cout << "\x1b[?25l\x1b[H" << renderBuffer_->buildDiffFrame() << std::flush;
}

bool UserLoginUI::isPrintableAscii(const char ch) {
    return std::isprint(static_cast<unsigned char>(ch)) != 0;
}

std::string UserLoginUI::maskPassword(const std::string &password) {
    return std::string(password.size(), '*');
}

} // namespace ui

