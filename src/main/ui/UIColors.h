#pragma once

// Shared inline helpers for converting ThemePalette colours to ftxui::Color.
// Include this header in every UI .cpp file instead of duplicating these two
// functions in each translation unit.

#include "ui/ThemeAdapter.h"

#include <ftxui/dom/elements.hpp>

namespace ui {

inline ftxui::Color toColor(const Rgb &rgb) {
    return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b);
}

// Returns Black or White, whichever is more readable on top of bg.
inline ftxui::Color highContrastOn(const Rgb &bg) {
    const int luma = (bg.r * 299 + bg.g * 587 + bg.b * 114) / 1000;
    return luma >= 140 ? ftxui::Color::Black : ftxui::Color::White;
}

} // namespace ui

