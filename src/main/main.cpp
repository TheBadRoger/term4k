#include "ui/MessageOverlay.h"
#include "ui/UIBus.h"

#include "utils/ErrorNotifier.h"

int main(int argc, char *argv[]) {
            ErrorNotifier::setSink([](const ErrorNotifier::Level level, const std::string &message) {
                ui::MessageLevel uiLevel = ui::MessageLevel::Info;
                if (level == ErrorNotifier::Level::Warning) uiLevel = ui::MessageLevel::Warning;
                if (level == ErrorNotifier::Level::Error) uiLevel = ui::MessageLevel::Error;
                ui::MessageOverlay::push(uiLevel, message);
            });

    (void)argc;
    (void)argv;

    return ui::UIBus::run();
}
