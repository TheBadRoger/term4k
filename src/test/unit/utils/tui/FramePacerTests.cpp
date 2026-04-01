#include "catch_amalgamated.hpp"

#include "utils/tui/FramePacer.h"

#include <chrono>

TEST_CASE("FramePacer drops rate after consecutive heavy frames", "[utils][tui][FramePacer]") {
    tui::FramePacer pacer(120);

    for (int i = 0; i < 8; ++i) {
        pacer.reportRenderCost(std::chrono::microseconds(20000));
    }
    REQUIRE(pacer.currentFps() == 60);

    for (int i = 0; i < 8; ++i) {
        pacer.reportRenderCost(std::chrono::microseconds(25000));
    }
    REQUIRE(pacer.currentFps() == 30);
}

TEST_CASE("FramePacer recovers rate after long stable period", "[utils][tui][FramePacer]") {
    tui::FramePacer pacer(120);

    for (int i = 0; i < 16; ++i) {
        pacer.reportRenderCost(std::chrono::microseconds(30000));
    }
    REQUIRE(pacer.currentFps() == 30);

    for (int i = 0; i < 180; ++i) {
        pacer.reportRenderCost(std::chrono::microseconds(2000));
    }
    REQUIRE(pacer.currentFps() == 60);

    for (int i = 0; i < 180; ++i) {
        pacer.reportRenderCost(std::chrono::microseconds(2000));
    }
    REQUIRE(pacer.currentFps() == 120);
}

