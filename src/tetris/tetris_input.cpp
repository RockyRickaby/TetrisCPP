#include <SDL3/SDL_scancode.h>
#include <array>
// #include <iostream>
#include "tetris_input.hpp"

namespace Tetris {
    bool Keybinds::setup_keys(Settings bind_settings) {
        if (bind_settings == Settings::Default) {
            keys.left.scancode = SDL_SCANCODE_LEFT;
            keys.right.scancode = SDL_SCANCODE_RIGHT;
            keys.down.scancode = SDL_SCANCODE_DOWN;
            keys.hard_drop.scancode = SDL_SCANCODE_UP;
        } else if (bind_settings == Settings::IJKL) {
            keys.left.scancode = SDL_SCANCODE_J;
            keys.right.scancode = SDL_SCANCODE_L;
            keys.down.scancode = SDL_SCANCODE_K;
            keys.hard_drop.scancode = SDL_SCANCODE_I;
        }
        keys.rotate_counterclockwise.scancode = SDL_SCANCODE_D;
        keys.rotate_clockwise.scancode = SDL_SCANCODE_F; 
        keys.hold_piece.scancode = SDL_SCANCODE_SPACE;
        keys.pause.scancode = SDL_SCANCODE_ESCAPE;

        keys.rotate_counterclockwise.may_repeat = false;
        keys.rotate_clockwise.may_repeat = false;
        
        keys.down.set_repeat_delay(0);
        keys.down.set_repeat_interval(0.05);

        return true;
    }

    bool Keybinds::any_match(SDL_Scancode scancode) const {
        constexpr static std::array keys_arr = {
            &TetrisKeys::left,
            &TetrisKeys::right,
            &TetrisKeys::down,
            &TetrisKeys::rotate_clockwise,
            &TetrisKeys::rotate_counterclockwise,
            &TetrisKeys::hard_drop,
            &TetrisKeys::hold_piece,
            &TetrisKeys::pause
        };
        for (const auto k : keys_arr) {
            if ((keys.*k).scancode == scancode) {
                return true;
            }
        }
        return false;
    }
}