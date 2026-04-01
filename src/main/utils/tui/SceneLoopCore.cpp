#include "utils/tui/SceneLoopCore.h"

#include "utils/tui/FramePacer.h"

#include <chrono>

namespace tui {

SceneLoopCore::~SceneLoopCore() {
    stop();
}

bool SceneLoopCore::start(const Hook &onTick, const int targetFps, const Hook &onStart, const Hook &onStop) {
    if (running_ || !onTick) return false;

    running_ = true;
    currentFps_ = targetFps;
    worker_ = std::thread([this, onTick, targetFps, onStart, onStop]() {
        FramePacer pacer(targetFps);
        pacer.begin();
        currentFps_ = pacer.currentFps();

        if (onStart) onStart();

        while (running_) {
            const auto tickStart = std::chrono::steady_clock::now();
            onTick();
            const auto tickCost = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - tickStart);
            pacer.reportRenderCost(tickCost);
            currentFps_ = pacer.currentFps();
            pacer.waitUntilNextFrame();
        }

        if (onStop) onStop();
    });

    return true;
}

void SceneLoopCore::requestStop() {
    running_ = false;
}

void SceneLoopCore::stop() {
    running_ = false;
    if (!worker_.joinable()) return;

    if (worker_.get_id() == std::this_thread::get_id()) return;
    worker_.join();
}

bool SceneLoopCore::running() const {
    return running_;
}

int SceneLoopCore::currentFps() const {
    return currentFps_;
}

} // namespace tui


