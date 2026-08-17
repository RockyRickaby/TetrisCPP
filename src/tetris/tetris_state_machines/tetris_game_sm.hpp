#pragma once

#include <SDL3/SDL.h>

#include "../../engine/state_machine.hpp"
// #include "../../engine/tengine.hpp"
#include "../../engine/events.hpp"
#include "../../engine/text/fonts.hpp"
#include "../tetris_game.hpp"
#include "../tetris_input.hpp"

namespace Tetris::States {
    inline static const int STATE_MAIN_MENU = 0;
    inline static const int STATE_TETRIS = 1;
    inline static const int STATE_GAMEOVER = 2;

    class TetrisStateMachine;

    // TODO - accept Tetris::Menu instance!!!!
    class MenuState final : public TEngine::StateMachine::State {
    public:
        MenuState(TetrisStateMachine *sm, Tetris::Keybinds* tetris_keys, SDL_Renderer *renderer, TEngine::Text::BitmapFonts::Font* font) :
            m_sm_ptr{sm},
            m_tetris_keys(tetris_keys),
            m_font{font},
            m_renderer{renderer}
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
        Tetris::Keybinds *m_tetris_keys;
        TEngine::Text::BitmapFonts::Font* m_font;
        SDL_Renderer *m_renderer;
        double tmp_counter = 5;
    };

    class RunGameState final : public TEngine::StateMachine::State {
    public:
        RunGameState(TetrisStateMachine *sm, Game *game, Tetris::Keybinds* tetris_keys, SDL_Renderer *renderer) :
            m_game_ptr{game},
            m_parent_sm(sm),
            m_keyboard_state{},
            m_tetris_keys(tetris_keys),
            m_renderer{renderer}
        {}

        void enter() override;
        void update(double delta_t) override;
        // void handle_input() override;
        void event(TEngine::Events::IEvent& event) override;
        void draw() override;
        void reset(void) override;
        void exit() override;

    private:
        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event) override;

        Game *m_game_ptr;
        TetrisStateMachine *m_parent_sm;
        TEngine::InputHandler m_keyboard_state;
        Tetris::Keybinds* m_tetris_keys;
        SDL_Renderer *m_renderer;
    };

    class GameOverState final : public TEngine::StateMachine::State {
    public:
        GameOverState(TetrisStateMachine *sm, SDL_Renderer *renderer, TEngine::Text::BitmapFonts::Font* font) :
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
        TEngine::Text::BitmapFonts::Font* m_font;
        SDL_Renderer *m_renderer;
        double tmp_counter = 5;
    };

    // dedicated state machine for handling everything needed for the game to run
    class TetrisStateMachine : public TEngine::StateMachine::StateMachine {
    public:
        TetrisStateMachine(
            Tetris::Game* game_ptr, 
            SDL_Renderer* renderer,
            Tetris::Keybinds* tetris_keys,
            TEngine::Text::BitmapFonts::Font* font
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