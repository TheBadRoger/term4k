#include "ui/HomePageUI.h"
#include "ui/ChartSelectUI.h"
#include "ui/SettingsUI.h"
#include "ui/UserInfoUI.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    while (true) {
        const int action = ui::HomePageUI::run();
        if (action == 1) {
            ui::ChartSelectUI::run();
            continue;
        }
        if (action == 2) {
            ui::SettingsUI::run();
            continue;
        }
        if (action == 5) {
            ui::UserInfoUI::run();
            continue;
        }
        break;
    }

    return 0;
}
