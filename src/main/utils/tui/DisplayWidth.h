#pragma once

#include <cstddef>
#include <string>

namespace tui {

// UTF-8 display width utilities for terminal rendering.
std::size_t displayWidth(const std::string &text);

// Clips UTF-8 text to fit the target terminal cell width.
std::string clipToDisplayWidth(const std::string &text, std::size_t maxWidth);

} // namespace tui

