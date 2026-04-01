#pragma once

#include "instances/UserLoginInstance.h"
#include "utils/tui/RenderBuffer.h"
#include "utils/tui/SceneLoopCore.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ui {

// Placeholder UI interface for user login screen.
class UserLoginUI {
public:
    explicit UserLoginUI(UserLoginInstance *instance = nullptr);

    void bindInstance(UserLoginInstance *instance);
    void show();
    void hide();

    void setUsernameInput(const std::string &username);

    void setPasswordInput(const std::string &password);

    void setSubmitModeLogin(bool loginMode);

    bool submit(std::string *outError = nullptr);

    bool consumeSubmitSucceeded();

    bool registerUser(const std::string &username, const std::string &password, std::string *outError = nullptr);

    bool login(const std::string &username, const std::string &password, std::string *outError = nullptr);

    void logout();

    bool isLoggedIn() const;

    std::optional<User> currentUser() const;

    const std::string &lastError() const;

private:
    enum class ActiveField { Username, Password };

    enum class SubmitMode { Login, Register };

    void loopOnce();

    void handleInputBytes(const std::string &chunk);

    bool submitCurrentInput();

    void renderFrame();

    static bool isPrintableAscii(char ch);

    static std::string maskPassword(const std::string &password);

    mutable std::string lastError_;
    std::string statusLine_;
    std::string usernameInput_;
    std::string passwordInput_;
    ActiveField activeField_ = ActiveField::Username;
    SubmitMode submitMode_ = SubmitMode::Login;
    std::size_t blinkCounter_ = 0;
    bool submitSucceeded_ = false;
    std::unique_ptr<tui::RenderBuffer> renderBuffer_;
    tui::SceneLoopCore loopCore_;
    UserLoginInstance *instance_ = nullptr;
};

} // namespace ui

