#pragma once

#include <chrono>

namespace tui {

// Controls frame pacing with adaptive fallback: 120 -> 60 -> 30 FPS.
class FramePacer {
public:
    explicit FramePacer(int targetFps = 120);

    void begin();

    void waitUntilNextFrame();

    void reportRenderCost(std::chrono::microseconds cost);

    int currentFps() const;

private:
    static int clampRate(int fps);

    static int lowerRate(int fps);

    static int higherRate(int fps);

    int targetFps_;
    int currentFps_;
    int heavyFrameStreak_ = 0;
    int lightFrameStreak_ = 0;
    std::chrono::steady_clock::time_point nextTick_;
};

} // namespace tui

