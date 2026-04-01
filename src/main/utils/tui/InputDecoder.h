#pragma once

#include <string>
#include <vector>

namespace tui {

enum class InputAction {
    None,
    MoveUp,
    MoveDown,
    WheelUp,
    WheelDown,
    Confirm,
    Quit,
};

class InputDecoder {
public:
    std::vector<InputAction> decode(const std::string &bytes) const;
};

} // namespace tui

