#include "ui/UserInfoUI.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "instances/AdminStatInstance.h"
#include "instances/UserLoginInstance.h"
#include "instances/UserStatInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <cstdio>
#include <string>

namespace ui {
namespace {

ftxui::Color toColor(const Rgb &rgb) { return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b); }

std::string formatDouble(const double value) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3f", value);
    return {buf};
}

enum class AuthMode { Login, Register };

} // namespace

int UserInfoUI::run() {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) { return I18nService::instance().get(key); };

    ThemePalette palette = ThemeAdapter::resolveFromRuntime();
    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());

    bool isAdmin = false;
    bool hasUser = false;
    bool isGuest = true;
    std::optional<User> current;

    UserStatInstance userStats;
    AdminStatInstance adminStats;
    UserLoginInstance loginInstance;

    std::string username;
    std::string password;
    std::string authStatus;
    int authFocus = 0;
    AuthMode authMode = AuthMode::Login;

    auto refreshSession = [&] {
        hasUser = AuthenticatedUserService::hasLoggedInUser();
        isAdmin = AuthenticatedUserService::isAdminUser();
        isGuest = AuthenticatedUserService::isGuestUser() || !hasUser;
        current = AuthenticatedUserService::currentUser();

        if (isAdmin) {
            adminStats.refresh(AppDirs::chartsDir());
        } else if (!isGuest) {
            userStats.refresh(AppDirs::chartsDir());
        }
    };
    refreshSession();

    InputOption userOpt;
    userOpt.placeholder = tr("ui.login.username");
    auto usernameInput = Input(&username, userOpt);

    InputOption passOpt;
    passOpt.placeholder = tr("ui.login.password");
    passOpt.password = true;
    auto passwordInput = Input(&password, passOpt);

    auto authContainer = Container::Vertical({usernameInput, passwordInput});

    auto screen = ScreenInteractive::Fullscreen();
    auto root = Renderer(authContainer, [&] {
        if (isGuest) {
            const std::string title = authMode == AuthMode::Login
                                          ? tr("ui.auth.login_title")
                                          : tr("ui.auth.register_title");

            Element userRow = window(text(" " + tr("ui.login.username") + " "), usernameInput->Render()) |
                              color(toColor(authFocus == 0 ? palette.accentPrimary : palette.borderNormal)) |
                              bgcolor(toColor(palette.surfacePanel));
            Element passRow = window(text(" " + tr("ui.login.password") + " "), passwordInput->Render()) |
                              color(toColor(authFocus == 1 ? palette.accentPrimary : palette.borderNormal)) |
                              bgcolor(toColor(palette.surfacePanel));

            Element statusLine = authStatus.empty()
                                     ? text("")
                                     : text(authStatus) | color(toColor(palette.textMuted));

            return vbox({
                       filler(),
                       text(title) | bold | color(toColor(palette.accentPrimary)) | center,
                       text(tr("ui.user_info.auth_switch_hint")) | color(toColor(palette.textMuted)) | center,
                       text(""),
                       hbox({filler(), vbox({userRow, passRow, statusLine}), filler()}),
                       filler(),
                   }) |
                   bgcolor(toColor(palette.surfacePanel)) |
                   color(toColor(palette.textPrimary)) |
                   flex;
        }

        Element body;
        if (isAdmin) {
            const auto &verified = adminStats.playerStats(AdminRecordScope::VerifiedOnly);
            const auto &all = adminStats.playerStats(AdminRecordScope::AllRecords);

            std::size_t verifiedRecords = 0;
            for (const auto &item : verified) verifiedRecords += item.second.records.order.size();
            std::size_t allRecords = 0;
            for (const auto &item : all) allRecords += item.second.records.order.size();

            body = vbox({
                text(tr("ui.user_info.admin_badge")) | bold | color(toColor(palette.accentPrimary)),
                text(tr("ui.user_info.admin_users_verified") + std::to_string(verified.size())),
                text(tr("ui.user_info.admin_records_verified") + std::to_string(verifiedRecords)),
                text(tr("ui.user_info.admin_users_all") + std::to_string(all.size())),
                text(tr("ui.user_info.admin_records_all") + std::to_string(allRecords)),
            }) | color(toColor(palette.textPrimary));
        } else {
            const std::string userName = current.has_value() ? current->getUsername() : tr("ui.login.user_unknown");
            const std::string uid = current.has_value() ? std::to_string(current->getUID()) : "-";
            const auto &records = userStats.records();

            body = vbox({
                text(tr("ui.user_info.username") + userName),
                text(tr("ui.user_info.uid") + uid),
                text(tr("ui.user_info.rating") + formatDouble(userStats.rating())),
                text(tr("ui.user_info.potential") + formatDouble(userStats.potential())),
                text(tr("ui.user_info.record_count") + std::to_string(records.order.size())),
            }) | color(toColor(palette.textPrimary));
        }

        return vbox({
                   hbox({
                       text(tr("ui.user_info.title")) | bold | color(toColor(palette.accentPrimary)),
                       filler(),
                       text(tr("ui.user_info.hint")) | color(toColor(palette.textMuted)),
                   }),
                   separator(),
                   body | flex,
               }) |
               borderRounded |
               bgcolor(toColor(palette.surfaceBg)) |
               color(toColor(palette.textPrimary)) |
               flex;
    });

    auto app = CatchEvent(root, [&](const Event &event) {
        if (event == Event::Escape || event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (isGuest) {
            if (event == Event::Character('r') || event == Event::Character('R')) {
                authMode = authMode == AuthMode::Login ? AuthMode::Register : AuthMode::Login;
                authStatus.clear();
                username.clear();
                password.clear();
                authFocus = 0;
                return true;
            }
            if (event == Event::Tab || event == Event::ArrowUp || event == Event::ArrowDown) {
                authFocus = (authFocus + 1) % 2;
                return true;
            }
            if (event == Event::Return) {
                if (username.empty()) {
                    authFocus = 0;
                    return true;
                }
                if (password.empty()) {
                    authFocus = 1;
                    return true;
                }

                if (authMode == AuthMode::Login) {
                    if (loginInstance.login(username, password)) {
                        authStatus.clear();
                        refreshSession();
                    } else {
                        authStatus = tr("ui.user_info.auth_login_failed");
                        username.clear();
                        password.clear();
                        authFocus = 0;
                    }
                } else {
                    std::string err;
                    if (loginInstance.registerUser(username, password, &err)) {
                        authMode = AuthMode::Login;
                        authStatus.clear();
                        username.clear();
                        password.clear();
                        authFocus = 0;
                    } else {
                        authStatus = err.empty() ? tr("ui.login.result.register_failed") : err;
                    }
                }
                return true;
            }

            if (authFocus == 0) return usernameInput->OnEvent(event);
            return passwordInput->OnEvent(event);
        }

        return false;
    });

    screen.Loop(app);
    return 0;
}

} // namespace ui

