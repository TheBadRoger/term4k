#include "catch_amalgamated.hpp"

#include "utils/tui/SceneLoopCore.h"

#include <atomic>
#include <chrono>
#include <thread>

TEST_CASE("SceneLoopCore starts ticks and supports in-loop stop requests", "[utils][tui][SceneLoopCore]") {
    tui::SceneLoopCore loop;
    std::atomic<int> ticks = 0;

    REQUIRE(loop.start([&]() {
        const int now = ++ticks;
        if (now >= 4) loop.requestStop();
    }, 120));

    for (int i = 0; i < 100 && loop.running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    loop.stop();

    REQUIRE(ticks >= 4);
    REQUIRE_FALSE(loop.running());
    REQUIRE((loop.currentFps() == 120 || loop.currentFps() == 60 || loop.currentFps() == 30));
}

TEST_CASE("SceneLoopCore rejects duplicate starts", "[utils][tui][SceneLoopCore]") {
    tui::SceneLoopCore loop;

    REQUIRE(loop.start([]() {}, 60));
    REQUIRE_FALSE(loop.start([]() {}, 120));

    loop.requestStop();
    loop.stop();
    REQUIRE_FALSE(loop.running());
}

