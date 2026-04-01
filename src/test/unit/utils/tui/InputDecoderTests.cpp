#include "catch_amalgamated.hpp"

#include "utils/tui/InputDecoder.h"

TEST_CASE("InputDecoder parses keyboard navigation and confirm", "[utils][tui][InputDecoder]") {
    const tui::InputDecoder decoder;
    const auto actions = decoder.decode(std::string("k") + "j" + "\n" + "q" + "\x1b[A" + "\x1b[B");

    REQUIRE(actions == std::vector<tui::InputAction>{
                           tui::InputAction::MoveUp,
                           tui::InputAction::MoveDown,
                           tui::InputAction::Confirm,
                           tui::InputAction::Quit,
                           tui::InputAction::MoveUp,
                           tui::InputAction::MoveDown,
                   });
}

TEST_CASE("InputDecoder parses xterm wheel events", "[utils][tui][InputDecoder]") {
    const tui::InputDecoder decoder;
    const auto actions = decoder.decode("\x1b[<64;40;7M\x1b[<65;40;8M");

    REQUIRE(actions == std::vector<tui::InputAction>{
                           tui::InputAction::WheelUp,
                           tui::InputAction::WheelDown,
                   });
}

