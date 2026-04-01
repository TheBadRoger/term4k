#pragma once

#include "instances/SettingsInstance.h"
#include "utils/tui/RenderBuffer.h"
#include "utils/tui/SceneLoopCore.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ui {

// Placeholder UI interface for user settings screen.
class UserSettingsUI {
public:
    explicit UserSettingsUI(SettingsInstance *instance = nullptr);

    void bindInstance(SettingsInstance *instance);
    void show();
    void hide();

    std::vector<std::string> availableThemeIds() const;

    bool selectTheme(const std::string &themeId, std::string *outError = nullptr);

    bool importThemeFromFile(const std::string &sourceFilePath,
                             std::string *outThemeId = nullptr,
                             std::string *outError = nullptr);

    bool exportSelectedThemeToUserDir(std::string *outError = nullptr) const;

    bool consumeApplySucceeded();

    const std::string &lastError() const;

private:
    void loopOnce();

    void applyMove(int delta);

    void renderFrame();

    std::size_t selectedThemeIndex_ = 0;
    bool applySucceeded_ = false;
    mutable std::string lastError_;
    std::unique_ptr<tui::RenderBuffer> renderBuffer_;
    tui::SceneLoopCore loopCore_;
    SettingsInstance *instance_ = nullptr;
};

} // namespace ui

