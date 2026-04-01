#pragma once

#include <atomic>
#include <functional>
#include <thread>

namespace tui {

// Reusable scene loop shell with paced worker lifecycle.
class SceneLoopCore {
public:
    using Hook = std::function<void()>;

    SceneLoopCore() = default;

    ~SceneLoopCore();

    bool start(const Hook &onTick, int targetFps = 120, const Hook &onStart = {}, const Hook &onStop = {});

    // Safe from any thread, including the worker callback.
    void requestStop();

    void stop();

    bool running() const;

    int currentFps() const;

private:
    std::thread worker_;
    std::atomic<bool> running_ = false;
    std::atomic<int> currentFps_ = 0;
};

} // namespace tui


