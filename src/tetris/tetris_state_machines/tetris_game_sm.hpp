#pragma once

#include <SDL3/SDL.h>

#include "../../engine/state_machine.hpp"
// #include "../../engine/tengine.hpp"
#include "../../engine/events.hpp"
#include "../../engine/text/fonts.hpp"
#include "../tetris_game.hpp"
#include "../tetris_input.hpp"
#include "../tetris_menu.hpp"

namespace Tetris::States {
    inline static const int STATE_MAIN_MENU = 0;
    inline static const int STATE_TETRIS = 1;
    inline static const int STATE_GAMEOVER = 2;

    class TetrisStateMachine;

    class MenuState final : public TEngine::StateMachine::State {
    public:
        MenuState(TetrisStateMachine *sm, MainMenu* menu, Tetris::Keybinds* tetris_keys, SDL_Renderer *renderer, TEngine::Text::BitmapFont* font) :
            m_sm_ptr{sm},
            m_menu{menu},
            m_tetris_keys(tetris_keys),
            m_font{font},
            m_renderer{renderer},
            tmp_counter{10},
            m_read_any{true}
        {}

        void enter(void) override;
        void update(double delta_t) override;
        // void handle_input(void) override;
        void event(TEngine::Events::IEvent& event) override;
        void draw(void) override;
        void reset(void) override;
        void exit(void) override;
    private:
        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event) override;

        TetrisStateMachine *m_sm_ptr;
        MainMenu* m_menu;
        Tetris::Keybinds *m_tetris_keys;
        TEngine::Text::BitmapFont* m_font;
        SDL_Renderer *m_renderer;
        double tmp_counter;
        bool m_read_any;
    };









    class RunGameState final : public TEngine::StateMachine::State {
    public:
        RunGameState(TetrisStateMachine* sm, Game* game, Tetris::Keybinds* tetris_keys, TEngine::Text::BitmapFont* font, SDL_Renderer* renderer) :
            m_run_state(),
            m_game_ptr{game},
            m_parent_sm(sm),
            m_tetris_keys(tetris_keys),
            m_renderer{renderer},
            m_font{font},
            m_keyboard_state{},
            m_begin_countdown{3},
            m_pause_countdown{0},
            m_pause{false}
        {}

        void enter() override;
        void update(double delta_t) override;
        // void handle_input() override;
        void event(TEngine::Events::IEvent& event) override;
        void draw() override;
        void reset(void) override;
        void exit() override;

    private:
        // not needed for now
        enum class State {
            Begin,
            Running,
            Pause
        };
        State m_run_state;

        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event) override;

        Game *m_game_ptr;
        TetrisStateMachine *m_parent_sm;
        Tetris::Keybinds* m_tetris_keys;
        SDL_Renderer *m_renderer;
        TEngine::Text::BitmapFont* m_font;
        TEngine::InputHandler m_keyboard_state;

        TEngine::Countdown m_begin_countdown;
        TEngine::Countdown m_pause_countdown;
        bool m_pause;
    };









    class GameOverState final : public TEngine::StateMachine::State {
    public:
        GameOverState(TetrisStateMachine *sm, SDL_Renderer *renderer, TEngine::Text::BitmapFont* font) :
            m_sm_ptr{sm},
            // m_input_event_handler(input_event_h),
            m_font{font},
            m_renderer{renderer}
        {}

        void enter(void) override;
        void update(double delta_t) override;
        // void handle_input(void) override;
        void draw(void) override;
        void reset(void) override;
        void exit(void) override;
    private:
        TetrisStateMachine *m_sm_ptr;
        TEngine::Text::BitmapFont* m_font;
        SDL_Renderer *m_renderer;
        double tmp_counter = 5;
    };









    // dedicated state machine for handling everything needed for the game to run
    class TetrisStateMachine : public TEngine::StateMachine::StateMachine {
    public:
        TetrisStateMachine(
            Tetris::Game* game_ptr,
            Tetris::MainMenu* menu_ptr,
            SDL_Renderer* renderer,
            Tetris::Keybinds* tetris_keys,
            TEngine::Text::BitmapFont* font
        );
        void update(double delta_t) override;
        // void handle_input(void) override { m_current->handle_input(); }
        void event(TEngine::Events::IEvent& event) override { m_current->event(event); }
        void draw(void) override { m_current->draw(); }
        void switch_to(int id);
    private:
        void update_state(void);

        RunGameState m_runstate;
        MenuState m_menustate;
        GameOverState m_gameoverstate;

        std::array<TEngine::StateMachine::State*, 3> m_states;
        TEngine::StateMachine::State* m_current;
        bool m_switching;
        int m_next;
    };
}