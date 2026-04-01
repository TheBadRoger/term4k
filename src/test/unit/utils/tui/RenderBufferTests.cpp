#include "catch_amalgamated.hpp"

#include "utils/tui/RenderBuffer.h"

TEST_CASE("RenderBuffer outputs ANSI only for changed lines", "[utils][tui][RenderBuffer]") {
    tui::RenderBuffer buffer(20, 4);

    buffer.clear();
    buffer.setLine(0, "header");
    buffer.setLine(2, "line-2");
    const std::string first = buffer.buildDiffFrame();

    REQUIRE(first.find("\x1b[1;1H") != std::string::npos);
    REQUIRE(first.find("header") != std::string::npos);
    REQUIRE(first.find("\x1b[3;1H") != std::string::npos);

    buffer.clear();
    buffer.setLine(0, "header");
    buffer.setLine(2, "line-2-updated");
    const std::string second = buffer.buildDiffFrame();

    REQUIRE(second.find("\x1b[1;1H") == std::string::npos);
    REQUIRE(second.find("\x1b[3;1H") != std::string::npos);
    REQUIRE(second.find("line-2-updated") != std::string::npos);
}

