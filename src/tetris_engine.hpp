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

    struct GameInput {
        enum class Keybinds {
            KEYBIND_DEFAULT, // uses arrow keys
            KEYBIND_IJKL,
        };
        struct State {
            SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
            bool pressed = false;
            bool is_repeat = false;
        };
        bool has_input = false;
        // Tetris::Vec2 move_vec{};
        // Tetris::Tetrimino::Rotation rotation{};

        struct {
            State left{};
            State right{};
            State down{}; // TODO soft drop. should be continuous (?) (auto-repeat?) (DONE? kinda)
            State autorepeat{}; // might not be used. for allowing a continuous press to move the piece quickly. might be unnecessary
            State hard_drop{};
            State rotate_clockwise{};
            State rotate_counterclockwise{};
            State hold_piece{};
            State pause{};
        } key{};

        bool setup_keys(Keybinds bind_settings);
        void read_input(const SDL_KeyboardEvent &keyboard);
        // return a tuple in case we want to return more things later (unlikely)
        std::tuple<Tetris::Vec2, Tetris::Tetrimino::Rotation> handle_input(void);
        void reset_states(void);
    };
}