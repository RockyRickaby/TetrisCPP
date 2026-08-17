#pragma once

#include <SDL3/SDL.h>
#include "../engine/tengine.hpp"

namespace Tetris {
    // contains the default keys that are needed to play tetris.
    // to be used in conjunction with TEngine::InputHandler, as some keys require specific timings.
    // may also be used with EventListeners to check for a scancode
    // may be copied.
    // may be set up manually or by calling setup_keys(Settings).
    class Keybinds {
    private:
        struct TetrisKeys {
            TEngine::Key left{};
            TEngine::Key right{};
            TEngine::Key down{};
            TEngine::Key rotate_clockwise{};
            TEngine::Key rotate_counterclockwise{};

            TEngine::Key hard_drop{};
            TEngine::Key hold_piece{};
            TEngine::Key pause{};
        };
    public:
        enum class Settings {
            Default, // uses arrow keys
            IJKL,
        };
        TetrisKeys keys{};

        bool setup_keys(Settings bind_settings);
        bool any_match(SDL_Scancode scancode) const;
    };
}