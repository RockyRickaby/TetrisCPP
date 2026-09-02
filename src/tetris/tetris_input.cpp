#include <SDL3/SDL_scancode.h>
#include <array>
#include "tetris_input.hpp"

namespace Tetris {
    bool Keybinds::setup_keys(Settings bind_settings) {
        SDL_Scancode rotccw = SDL_SCANCODE_D;
        SDL_Scancode rotcw = SDL_SCANCODE_F; 
        SDL_Scancode hold = SDL_SCANCODE_SPACE;
        SDL_Scancode pause = SDL_SCANCODE_ESCAPE;
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
        } else if (bind_settings == Settings::Standard_compliant) {
            keys.left.scancode = SDL_SCANCODE_LEFT;
            keys.right.scancode = SDL_SCANCODE_RIGHT;
            keys.down.scancode = SDL_SCANCODE_DOWN;
            keys.hard_drop.scancode = SDL_SCANCODE_SPACE;

            rotccw = SDL_SCANCODE_Z;
            rotcw = SDL_SCANCODE_X; 
            hold = SDL_SCANCODE_C;
            pause = SDL_SCANCODE_ESCAPE;
        }
        keys.rotate_counterclockwise.scancode = rotccw;
        keys.rotate_clockwise.scancode = rotcw; 
        keys.hold_piece.scancode = hold;
        keys.pause.scancode = pause;

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