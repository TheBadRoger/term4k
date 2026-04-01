#include "utils/tui/RenderBuffer.h"

#include "utils/tui/DisplayWidth.h"

#include <sstream>

namespace tui {

RenderBuffer::RenderBuffer(const std::size_t width, const std::size_t height)
    : width_(width), height_(height), previous_(height), current_(height) {}

void RenderBuffer::clear() {
    for (std::string &line : current_) {
        line.clear();
    }
}

void RenderBuffer::setLine(const std::size_t row, const std::string &line) {
    if (row >= height_) return;
    current_[row] = clip(line);
}

std::string RenderBuffer::buildDiffFrame() {
    std::ostringstream ansi;

    for (std::size_t row = 0; row < height_; ++row) {
        if (current_[row] == previous_[row]) continue;
        ansi << "\x1b[" << (row + 1) << ";1H";
        ansi << "\x1b[2K";
        ansi << current_[row];
    }

    previous_ = current_;
    return ansi.str();
}

std::string RenderBuffer::clip(const std::string &line) const {
    return clipToDisplayWidth(line, width_);
}

} // namespace tui

