#include "utils/tui/InputDecoder.h"

namespace tui {

std::vector<InputAction> InputDecoder::decode(const std::string &bytes) const {
    std::vector<InputAction> actions;

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const char ch = bytes[i];

        if (ch == 'k') {
            actions.push_back(InputAction::MoveUp);
            continue;
        }
        if (ch == 'j') {
            actions.push_back(InputAction::MoveDown);
            continue;
        }
        if (ch == 'q') {
            actions.push_back(InputAction::Quit);
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            actions.push_back(InputAction::Confirm);
            continue;
        }

        if (ch == '\x1b' && i + 2 < bytes.size() && bytes[i + 1] == '[') {
            const char code = bytes[i + 2];
            if (code == 'A') {
                actions.push_back(InputAction::MoveUp);
                i += 2;
                continue;
            }
            if (code == 'B') {
                actions.push_back(InputAction::MoveDown);
                i += 2;
                continue;
            }

            // XTerm SGR mouse wheel: ESC [ <64;...M (up), ESC [ <65;...M (down)
            if (code == '<') {
                const std::size_t tail = bytes.find('M', i + 3);
                if (tail != std::string::npos) {
                    const std::string payload = bytes.substr(i + 3, tail - (i + 3));
                    if (payload.rfind("64;", 0) == 0) actions.push_back(InputAction::WheelUp);
                    if (payload.rfind("65;", 0) == 0) actions.push_back(InputAction::WheelDown);
                    i = tail;
                    continue;
                }
            }
        }
    }

    return actions;
}

} // namespace tui

