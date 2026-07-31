#include "tetris_engine.hpp"

namespace Tetris::Engine {
    static const bool *keyboard = nullptr;

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
        key.rotate_counterclockwise.scancode = SDL_SCANCODE_D; // FIXME - should be Z, LCTRL AND/or RCTRL
        key.rotate_clockwise.scancode = SDL_SCANCODE_F; // FIXME - should be X or UP key
        key.hold_piece.scancode = SDL_SCANCODE_SPACE; // FIXME - should be LSHIFT, RSHIFT AND/or C
        key.pause.scancode = SDL_SCANCODE_P; // FIXME - should be ESC or F1

        key.rotate_counterclockwise.may_repeat = false;
        key.rotate_clockwise.may_repeat = false;

        keyboard = SDL_GetKeyboardState(nullptr);
        return true;
    }

    void GameInput::read_input_event(const SDL_KeyboardEvent &keyboard) {
        if (keyboard.down) {
            // only save the state of one key at a time
            if (!keyboard.repeat) {
                SDL_Scancode code = keyboard.scancode;
                key.hard_drop.pressed = code == key.hard_drop.scancode;
                key.hold_piece.pressed = code == key.hold_piece.scancode;
                key.pause.pressed = code == key.pause.scancode;
            } else {
                // these should not ever repeat
                // key.hard_drop.pressed = false;
                // key.rotate_clockwise.pressed = false;
                // key.rotate_counterclockwise.pressed = false;               
            }
            has_input_event = true; // allows for repeat presses to be read without changing their states
        } else {
            has_input_event = false;
        }
    }

    // std::tuple<Vec2, Tetrimino::Rotation> GameInput::read_input_event(void) {
    //     using Tetrimino::Rotation;
    //     if (!has_input_event) {
    //         reset_states();
    //     }
    //     auto pair = std::make_tuple(Vec2{}, Rotation::NONE);
    //     // process one at a time. no diagonals allowed
    //     if (key.left.pressed) {
    //         std::get<Vec2>(pair) = Vec2{-1,0};
    //     } else if (key.right.pressed) {
    //         std::get<Vec2>(pair) = Vec2{1,0};
    //     } else if (key.down.pressed) {
    //         std::get<Vec2>(pair) = Vec2{0,-1};
    //     }
        
    //     if (key.rotate_clockwise.pressed) {
    //         std::get<Rotation>(pair) = Rotation::CLOCKWISE;
    //     } else if (key.rotate_counterclockwise.pressed) {
    //         std::get<Rotation>(pair) = Rotation::COUNTERCLOCKWISE;
    //     } else {
    //     }
    //     return pair;
    // }
    
    std::tuple<Vec2, Tetrimino::Rotation> GameInput::read_input_state(void) {
        // std::cout << "READING STATE\n";
        using Tetrimino::Rotation;
        Vec2 dir{};
        Rotation rot{};
        // FIXME - if <- is currently pressed and -> is then pressed at the same time, -> should be prioritized over <-.
        // the opposite should also happen
        if (may_press(key.left)) {
            dir = Vec2{-1,0};
        } else if (may_press(key.right)) {
            dir = Vec2{1,0};
        } else if (may_press(key.down)) {
            dir = Vec2{0,-1};
        }
        
        if (may_press(key.rotate_clockwise)) {
            rot = Rotation::CLOCKWISE;
        } else if (may_press(key.rotate_counterclockwise)) {
            rot = Rotation::COUNTERCLOCKWISE;
        } else {
        }

        return std::make_tuple(dir, rot);
    }

    void GameInput::reset_events(void) {
        has_input_event = false;
        key.hard_drop.pressed = false;
        key.hold_piece.pressed = false;
        key.pause.pressed = false;
    }

    bool GameInput::may_press(KeyState &key) {
        switch (key.m_keystate) {
            case __KeyState::KEYSTATE_UP: {
                if (keyboard[key.scancode]) {
                    key.m_keystate = __KeyState::KEYSTATE_PRESSED;
                    return true;
                }
            }; break;
            case __KeyState::KEYSTATE_PRESSED: {
                if (keyboard[key.scancode]) {
                    if (!key.may_repeat) {
                        return false;
                    }
                    key.m_keystate = __KeyState::KEYSTATE_WAIT;
                    key.m_repeat_delay.done(key.m_timer.delta_time());
                } else {
                    key.m_keystate = __KeyState::KEYSTATE_UP;
                }
                return false;
            }; break;
            case __KeyState::KEYSTATE_WAIT: {
                if (keyboard[key.scancode]) {
                    if (!key.m_repeat_delay.done(key.m_timer.delta_time())) {
                        return false;
                    }
                    key.m_keystate = __KeyState::KEYSTATE_REPEAT;
                    return true;
                } else {
                    key.m_keystate = __KeyState::KEYSTATE_UP;
                    key.m_repeat_delay.reset();
                    return false;
                }
            }; break;
            case __KeyState::KEYSTATE_REPEAT: {
                if (!keyboard[key.scancode]) {
                    key.m_keystate = __KeyState::KEYSTATE_UP;
                    key.m_repeat_interval.reset();
                    return false;
                } else if (!key.m_repeat_interval.done(key.m_timer.delta_time())) {
                    return false;
                } else {
                    return true;
                }
            }; break;
            default:
                return false;
        }
        return false;
    }
}