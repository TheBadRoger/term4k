#include "ui/HomePageUI.h"
#include "ui/ChartSelectUI.h"
#include "ui/MessageOverlay.h"
#include "ui/SettingsUI.h"
#include "ui/UserInfoUI.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "utils/ErrorNotifier.h"
#include "services/I18nService.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/screen_interactive.hpp>

namespace {

void prepareHomePage() {
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    (void)ui::ThemeAdapter::resolveFromRuntime();
}

void prepareChartSelect() {
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    (void)ui::ThemeAdapter::resolveFromRuntime();
    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());
}

void prepareSettings() {
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    (void)ui::ThemeAdapter::resolveFromRuntime();
}

void prepareUserInfo() {
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    (void)ui::ThemeAdapter::resolveFromRuntime();
    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());
}

} // namespace

int main(int argc, char *argv[]) {
            ErrorNotifier::setSink([](const ErrorNotifier::Level level, const std::string &message) {
                ui::MessageLevel uiLevel = ui::MessageLevel::Info;
                if (level == ErrorNotifier::Level::Warning) uiLevel = ui::MessageLevel::Warning;
                if (level == ErrorNotifier::Level::Error) uiLevel = ui::MessageLevel::Error;
                ui::MessageOverlay::push(uiLevel, message);
            });

    (void)argc;
    (void)argv;

    auto screen = ftxui::ScreenInteractive::Fullscreen();

    bool firstHomeEntry = true;
    while (true) {
        if (!firstHomeEntry) {
            prepareHomePage();
        }
        firstHomeEntry = false;

        const int action = ui::HomePageUI::run(screen);
        if (action == 1) {
            prepareChartSelect();
            ui::ChartSelectUI::run(screen);
            continue;
        }
        if (action == 2) {
            prepareSettings();
            ui::SettingsUI::run(screen);
            continue;
        }
        if (action == 5) {
            prepareUserInfo();
            ui::UserInfoUI::run(screen);
            continue;
        }
        break;
    }

    return 0;
}
