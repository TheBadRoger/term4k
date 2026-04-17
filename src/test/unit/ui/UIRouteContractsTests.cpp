#include "catch_amalgamated.hpp"

#include "ui/UIBus.h"

TEST_CASE("StartMenu actions route to expected scenes", "[ui][UIRouteContracts]") {
    REQUIRE(ui::sceneUsesComponentLoop(ui::UIScene::StartMenu));
    REQUIRE(ui::sceneUsesComponentLoop(ui::UIScene::ChartList));
    REQUIRE(ui::sceneUsesComponentLoop(ui::UIScene::Settings));
    REQUIRE(ui::sceneUsesComponentLoop(ui::UIScene::UserStat));
    REQUIRE_FALSE(ui::sceneUsesComponentLoop(ui::UIScene::Exit));

    REQUIRE_FALSE(ui::sceneNeedsDataDirs(ui::UIScene::StartMenu));
    REQUIRE(ui::sceneNeedsDataDirs(ui::UIScene::ChartList));
    REQUIRE_FALSE(ui::sceneNeedsDataDirs(ui::UIScene::Settings));
    REQUIRE(ui::sceneNeedsDataDirs(ui::UIScene::UserStat));
    REQUIRE_FALSE(ui::sceneNeedsDataDirs(ui::UIScene::Exit));
}


