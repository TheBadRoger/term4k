#include "catch_amalgamated.hpp"

#include "utils/tui/DisplayWidth.h"

TEST_CASE("DisplayWidth handles ASCII and CJK width", "[utils][tui][DisplayWidth]") {
    REQUIRE(tui::displayWidth("abc") == 3);
    REQUIRE(tui::displayWidth("中") == 2);
    REQUIRE(tui::displayWidth("ab中") == 4);
}

TEST_CASE("DisplayWidth clip respects terminal cell width", "[utils][tui][DisplayWidth]") {
    REQUIRE(tui::clipToDisplayWidth("abcdef", 4) == "abcd");
    REQUIRE(tui::clipToDisplayWidth("ab中cd", 4) == "ab中");
    REQUIRE(tui::clipToDisplayWidth("中中a", 3) == "中");
}

