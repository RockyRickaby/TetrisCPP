#include "tetris_engine.hpp"

namespace Tetris::Engine {
    bool GameInput::setup_keys(Keybinds bind_settings) {
        if (bind_settings == Keybinds::KEYBIND_DEFAULT) {
            key.left.scancode = SDL_SCANCODE_LEFT;
            key.right.scancode = SDL_SCANCODE_RIGHT;
            key.down.scancode = SDL_SCANCODE_DOWN;
            key.hard_drop.scancode = SDL_SCANCODE_UP; // FIXME - should be spacebar according to specs... we may consider whether we should consider that
        } else if (bind_settings == Keybinds::KEYBIND_IJKL) {
            key.left.scancode = SDL_SCANCODE_J;
            key.right.scancode = SDL_SCANCODE_L;
            key.down.scancode = SDL_SCANCODE_K;
            key.hard_drop.scancode = SDL_SCANCODE_I;
        }
        key.autorepeat.scancode = SDL_SCANCODE_UNKNOWN; // FIXME- not needed
        key.rotate_counterclockwise.scancode = SDL_SCANCODE_D; // FIXME - should be Z, LCTRL AND/or RCTRL
        key.rotate_clockwise.scancode = SDL_SCANCODE_F; // FIXME - should be X or UP key
        key.hold_piece.scancode = SDL_SCANCODE_SPACE; // FIXME - should be LSHIFT, RSHIFT AND/or C
        key.pause.scancode = SDL_SCANCODE_P; // FIXME - should be ESC or F1
        return true;
    }

    void GameInput::read_input(const SDL_KeyboardEvent &keyboard) {
        // TODO - allow soft drop and auto-repeat for sideways movement (DONE? has_input = true basically does it automatically, and with a delay)
        if (keyboard.down) {
            // only save the state of one key at a time
            if (!keyboard.repeat) {
                SDL_Scancode code = keyboard.scancode;
                // if ((key.left.pressed || key.right.pressed || key.down.pressed) && (code == key.rotate_clockwise.scancode || code == key.rotate_counterclockwise.scancode)) {
                    // key.rotate_clockwise.pressed = code == key.rotate_clockwise.scancode;
                    // key.rotate_counterclockwise.pressed = code == key.rotate_counterclockwise.scancode;  
                // } else {
                    // TODO - set other keys' states
                    key.left.pressed = code == key.left.scancode;
                    key.right.pressed = code == key.right.scancode;
                    key.down.pressed = code == key.down.scancode;
                    key.hard_drop.pressed = code == key.hard_drop.scancode;
                    key.rotate_clockwise.pressed = code == key.rotate_clockwise.scancode;
                    key.rotate_counterclockwise.pressed = code == key.rotate_counterclockwise.scancode;
                // }
            } else {
                // these should not ever repeat
                // key.hard_drop.pressed = false;
                // key.rotate_clockwise.pressed = false;
                // key.rotate_counterclockwise.pressed = false;               
            }
            has_input = true; // allows for repeat presses to be read without changing their states
        } else {
            has_input = false;
        }
    }

    std::tuple<Vec2, Tetrimino::Rotation> GameInput::handle_input(void) {
        using Tetrimino::Rotation;
        if (!has_input) {
            reset_states();
        }
        auto pair = std::make_tuple(Vec2{}, Rotation::NONE);
        // process one at a time. no diagonals allowed
        if (key.left.pressed) {
            std::get<Vec2>(pair) = Vec2{-1,0};
        } else if (key.right.pressed) {
            std::get<Vec2>(pair) = Vec2{1,0};
        } else if (key.down.pressed) {
            std::get<Vec2>(pair) = Vec2{0,-1};
        }
        
        if (key.rotate_clockwise.pressed) {
            std::get<Rotation>(pair) = Rotation::CLOCKWISE;
        } else if (key.rotate_counterclockwise.pressed) {
            std::get<Rotation>(pair) = Rotation::COUNTERCLOCKWISE;
        } else {
        }
        return pair;
    }

    void GameInput::reset_states(void) {
        // move_vec = Tetris::Vec2{};
        // rotation = Tetris::Tetrimino::Rotation::NONE;
        has_input = false;
        // key = {};
        // key.left.pressed = false;
        // key.right.pressed = false;
        // key.down.pressed = false;
        key.autorepeat.pressed = false;
        key.hard_drop.pressed = false;
        key.rotate_clockwise.pressed = false;
        key.rotate_counterclockwise.pressed = false;
        key.hold_piece.pressed = false;
        key.pause.pressed = false;
        // key..pressed = false;
    }
}