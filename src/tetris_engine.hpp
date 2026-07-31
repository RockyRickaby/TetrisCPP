#pragma once

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_scancode.h>

#include "tetris_base.hpp"

namespace Tetris::Engine {
    // Constructor should be called after initializing SDL
    // Calculates the delta time between frames
    class Time {
    public:
        Time() :
            m_freq{SDL_GetPerformanceFrequency()},
            time_last{SDL_GetPerformanceCounter()}
        {}
        double delta_time(void) {
            Uint64 time_now = SDL_GetPerformanceCounter();
            double delta_time = (static_cast<double>(time_now - time_last) / static_cast<double>(m_freq));
            time_last = time_now;
            return delta_time;
        }
    private:
        Uint64 m_freq;
        Uint64 time_last;
    };

    // Countdown timer. Time is assumed to be in seconds
    struct Countdown {
        double m_time_counter;
        double m_time_delta;
        bool m_autoreset;

        Countdown(double count, bool autoreset = false) :
            m_time_counter{count},
            m_time_delta{count},
            m_autoreset{autoreset}
        {}
        bool done(double delta_t) {
            m_time_counter -= delta_t;
            if (m_time_counter <= 0) {
                if (m_autoreset) {
                    reset();
                } else {
                    m_time_counter += delta_t; // unlikely to happen, but prevent the value from getting tooooooo small
                }
                return true;
            }
            return false;
        }
        void reset(void) {
            m_time_counter = m_time_delta;
        }
    };

    // TODO - overhaul this to use GetKeyboardState().... maybe.
    // it would allow for some simultaneous actions to happen
    struct GameInput {
        enum class __KeyState {
            KEYSTATE_UP,
            KEYSTATE_PRESSED,
            KEYSTATE_WAIT,
            KEYSTATE_REPEAT
        };
        enum class Keybinds {
            KEYBIND_DEFAULT, // uses arrow keys
            KEYBIND_IJKL,
        };
        // never repeats
        struct KeyEvent {
            SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
            bool pressed = false;
        };
        // may or may not repeat
        struct KeyState {
            bool pressed = false;
            bool may_repeat = true; 
            SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;

            Countdown m_repeat_delay{0.3, true};
            Countdown m_repeat_interval{0.025, true};
            Time m_timer;
            __KeyState m_keystate{0};
        };
        struct {
            KeyState left{};
            KeyState right{};
            KeyState down{};
            KeyState rotate_clockwise{};
            KeyState rotate_counterclockwise{};

            KeyEvent hard_drop{};
            KeyEvent hold_piece{};
            KeyEvent pause{};
        } key{};
        
        bool has_input_event = false;

        bool setup_keys(Keybinds bind_settings);
        // for anything non-movement related
        void read_input_event(const SDL_KeyboardEvent &keyboard);
        // std::tuple<Tetris::Vec2, Tetris::Tetrimino::Rotation> read_input_event(void);
        // only for movement related actions
        std::tuple<Tetris::Vec2, Tetris::Tetrimino::Rotation> read_input_state(void); 
        void reset_events(void);
        
        // not intended for external usage, but will work just fine
        bool may_press(KeyState &key);
    };
}