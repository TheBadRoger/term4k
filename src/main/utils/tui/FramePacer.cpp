#include "utils/tui/FramePacer.h"

#include <algorithm>
#include <thread>

namespace tui {

namespace {
std::chrono::microseconds frameBudget(const int fps) {
    return std::chrono::microseconds(1000000 / std::max(1, fps));
}
} // namespace

FramePacer::FramePacer(const int targetFps)
    : targetFps_(clampRate(targetFps)), currentFps_(clampRate(targetFps)), nextTick_(std::chrono::steady_clock::now()) {}

void FramePacer::begin() {
    nextTick_ = std::chrono::steady_clock::now();
}

void FramePacer::waitUntilNextFrame() {
    const auto budget = frameBudget(currentFps_);
    nextTick_ += budget;

    const auto now = std::chrono::steady_clock::now();
    if (nextTick_ > now) {
        std::this_thread::sleep_until(nextTick_);
    } else {
        // Avoid drift explosion when frame time exceeds budget.
        nextTick_ = now;
    }
}

void FramePacer::reportRenderCost(const std::chrono::microseconds cost) {
    const auto budget = frameBudget(currentFps_);

    if (cost > budget) {
        ++heavyFrameStreak_;
        lightFrameStreak_ = 0;
    } else {
        ++lightFrameStreak_;
        heavyFrameStreak_ = 0;
    }

    if (heavyFrameStreak_ >= 8) {
        currentFps_ = lowerRate(currentFps_);
        heavyFrameStreak_ = 0;
        lightFrameStreak_ = 0;
        return;
    }

    if (lightFrameStreak_ >= 180 && currentFps_ < targetFps_) {
        currentFps_ = higherRate(currentFps_);
        lightFrameStreak_ = 0;
    }
}

int FramePacer::currentFps() const {
    return currentFps_;
}

int FramePacer::clampRate(const int fps) {
    if (fps >= 120) return 120;
    if (fps >= 60) return 60;
    return 30;
}

int FramePacer::lowerRate(const int fps) {
    if (fps > 60) return 60;
    if (fps > 30) return 30;
    return 30;
}

int FramePacer::higherRate(const int fps) {
    if (fps < 60) return 60;
    if (fps < 120) return 120;
    return 120;
}

} // namespace tui

