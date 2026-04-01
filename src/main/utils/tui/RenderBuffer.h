#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace tui {

// Maintains a previous frame snapshot and emits ANSI updates for changed lines only.
class RenderBuffer {
public:
    RenderBuffer(std::size_t width, std::size_t height);

    void clear();

    void setLine(std::size_t row, const std::string &line);

    std::string buildDiffFrame();

private:
    std::string clip(const std::string &line) const;

    std::size_t width_;
    std::size_t height_;
    std::vector<std::string> previous_;
    std::vector<std::string> current_;
};

} // namespace tui

