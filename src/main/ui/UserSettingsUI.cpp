#include "UserSettingsUI.h"

#include "config/RuntimeConfigs.h"
#include "services/I18nService.h"
#include "utils/ErrorNotifier.h"
#include "utils/tui/InputDecoder.h"
#include "utils/tui/RenderBuffer.h"

#include <chrono>
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

std::string readInputChunk() {
    char buffer[256] = {};
    const ssize_t n = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (n <= 0) return "";
    return std::string(buffer, static_cast<std::size_t>(n));
}

bool pollStdin(const int timeoutMs) {
    pollfd pfd{};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    const int rc = ::poll(&pfd, 1, timeoutMs);
    return rc > 0 && (pfd.revents & POLLIN);
}

std::string tr(const std::string &key) {
    auto &i18n = I18nService::instance();
    i18n.ensureLocaleLoaded(RuntimeConfigs::locale);
    return i18n.get(key);
}

void setErrorAndNotify(std::string &lastError, const std::string &key) {
    lastError = tr(key);
    ErrorNotifier::notify("UserSettingsUI", lastError);
}

std::string kvLine(const std::string &labelKey, const std::string &value) {
    return tr(labelKey) + ": " + value;
}
} // namespace

UserSettingsUI::UserSettingsUI(SettingsInstance *instance) : instance_(instance) {}

void UserSettingsUI::bindInstance(SettingsInstance *instance) {
    instance_ = instance;
}

void UserSettingsUI::show() {
    if (loopCore_.running()) return;
    renderBuffer_ = std::make_unique<tui::RenderBuffer>(100, 12);
    applySucceeded_ = false;

    loopCore_.start([this]() {
        static thread_local ScopedRawMode rawMode(STDIN_FILENO);
        loopOnce();
    }, 120);
}

void UserSettingsUI::hide() {
    loopCore_.stop();
    renderBuffer_.reset();

    // Restore cursor position and attributes for the next scene.
    std::cout << "\x1b[0m\x1b[?25h" << std::flush;
}

std::vector<std::string> UserSettingsUI::availableThemeIds() const {
    if (instance_ == nullptr) return {};
    return instance_->availableThemeIds();
}

bool UserSettingsUI::selectTheme(const std::string &themeId, std::string *outError) {
    if (instance_ == nullptr) {
        setErrorAndNotify(lastError_, "ui.error.settings_instance_unbound");
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (!instance_->selectTheme(themeId, &lastError_)) {
        ErrorNotifier::notify("UserSettingsUI", lastError_);
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (outError != nullptr) outError->clear();
    lastError_.clear();
    applySucceeded_ = true;
    return true;
}

bool UserSettingsUI::importThemeFromFile(const std::string &sourceFilePath,
                                         std::string *outThemeId,
                                         std::string *outError) {
    if (instance_ == nullptr) {
        setErrorAndNotify(lastError_, "ui.error.settings_instance_unbound");
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (!instance_->importThemeFromFile(sourceFilePath, outThemeId, &lastError_)) {
        ErrorNotifier::notify("UserSettingsUI", lastError_);
        if (outError != nullptr) *outError = lastError_;
        return false;
    }

    if (outError != nullptr) outError->clear();
    lastError_.clear();
    applySucceeded_ = true;
    return true;
}

bool UserSettingsUI::exportSelectedThemeToUserDir(std::string *outError) const {
    if (instance_ == nullptr) {
        const std::string message = tr("ui.error.settings_instance_unbound");
        ErrorNotifier::notify("UserSettingsUI", message);
        if (outError != nullptr) *outError = message;
        return false;
    }

    std::string error;
    if (!instance_->exportSelectedThemeToUserDir(&error)) {
        ErrorNotifier::notify("UserSettingsUI", error);
        if (outError != nullptr) *outError = error;
        return false;
    }

    if (outError != nullptr) outError->clear();
    return true;
}

bool UserSettingsUI::consumeApplySucceeded() {
    const bool applied = applySucceeded_;
    applySucceeded_ = false;
    return applied;
}

const std::string &UserSettingsUI::lastError() const {
    return lastError_;
}

void UserSettingsUI::loopOnce() {
    renderFrame();

    if (!pollStdin(0)) return;

    const std::string chunk = readInputChunk();
    if (chunk.empty()) return;

    const tui::InputDecoder decoder;
    const auto actions = decoder.decode(chunk);
    const auto themes = availableThemeIds();

    for (const tui::InputAction action : actions) {
        switch (action) {
            case tui::InputAction::MoveUp:
            case tui::InputAction::WheelUp:
                applyMove(-1);
                break;
            case tui::InputAction::MoveDown:
            case tui::InputAction::WheelDown:
                applyMove(1);
                break;
            case tui::InputAction::Confirm:
                if (!themes.empty()) {
                    std::string ignored;
                    selectTheme(themes[selectedThemeIndex_], &ignored);
                }
                break;
            case tui::InputAction::Quit:
                loopCore_.requestStop();
                break;
            default:
                break;
        }
    }
}

void UserSettingsUI::applyMove(const int delta) {
    const auto themes = availableThemeIds();
    if (themes.empty()) {
        selectedThemeIndex_ = 0;
        return;
    }

    const std::size_t size = themes.size();
    const int current = static_cast<int>(selectedThemeIndex_);
    int next = current + delta;

    if (next < 0) next = static_cast<int>(size) - 1;
    if (next >= static_cast<int>(size)) next = 0;

    selectedThemeIndex_ = static_cast<std::size_t>(next);
}

void UserSettingsUI::renderFrame() {
    if (!renderBuffer_) return;

    const auto themes = availableThemeIds();

    if (!themes.empty() && selectedThemeIndex_ >= themes.size()) {
        selectedThemeIndex_ = themes.size() - 1;
    }

    renderBuffer_->clear();
    const int fps = loopCore_.currentFps();
    renderBuffer_->setLine(0, tr("ui.settings.title"));
    renderBuffer_->setLine(1, tr("ui.settings.hint"));
    renderBuffer_->setLine(2, kvLine("ui.common.scene_pacing", std::to_string(fps) + " " + tr("ui.common.fps_unit")));

    if (themes.empty()) {
        renderBuffer_->setLine(4, tr("ui.settings.no_themes"));
    } else {
        for (std::size_t i = 0; i < themes.size() && i < 8; ++i) {
            const std::string prefix = (i == selectedThemeIndex_) ? "> " : "  ";
            renderBuffer_->setLine(i + 4, prefix + themes[i]);
        }
    }

    if (!lastError_.empty()) {
        renderBuffer_->setLine(11, kvLine("ui.common.error", lastError_));
    }

    std::cout << "\x1b[?25l\x1b[H" << renderBuffer_->buildDiffFrame() << std::flush;
}

} // namespace ui

