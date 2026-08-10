#pragma once

#include <SDL3/SDL.h>

#include "../../engine/state_machine.hpp"
#include "../../engine/tengine.hpp"
#include "../tetris_game.hpp"

namespace Tetris::States {
    inline static const int STATE_MAIN_MENU = 0xA;
    inline static const int STATE_TETRIS = 0xB;
    inline static const int STATE_GAMEOVER = 0XC;

    class MenuState final : public TEngine::StateMachine::State {
    public:
        MenuState(TEngine::StateMachine::GenericStateMachine<int> *sm, SDL_Renderer *renderer, TEngine::GameInputEventHandler *input_event_h, TEngine::GameInputStateHandler *input_state_h) :
            m_sm_ptr{sm},
            m_input_state_handler(input_state_h),
            m_input_event_handler(input_event_h),
            m_renderer{renderer}
        {}

        void enter(void) override;
        void update(double delta_t) override;
        // void handle_input_event(GameInputEventHandler& input_handler) override;
        // void handle_input_state(GameInputStateHandler& input_handler) override;
        void handle_input(void) override;
        // void draw(SDL_Renderer *renderer) override;
        void draw(void) override;
        void reset(void) override;
        void exit(void) override;
    private:
        TEngine::StateMachine::GenericStateMachine<int> *m_sm_ptr;
        TEngine::GameInputStateHandler *m_input_state_handler;
        TEngine::GameInputEventHandler *m_input_event_handler;
        SDL_Renderer *m_renderer;
        double tmp_counter = 5;
    };

    class RunGameState final : public TEngine::StateMachine::State {
    public:
        RunGameState(TEngine::StateMachine::GenericStateMachine<int> *sm, Game *game, SDL_Renderer *renderer, TEngine::GameInputEventHandler *input_event_h, TEngine::GameInputStateHandler *input_state_h) :
            m_game_ptr{game},
            m_parent_sm(sm),
            m_input_state_handler(input_state_h),
            m_input_event_handler(input_event_h),
            m_renderer{renderer}
        {}

        void enter() override;
        void update(double delta_t) override;
        void handle_input() override;
        void draw() override;
        void reset(void) override;
        void exit() override;

    private:
        Game *m_game_ptr;
        TEngine::StateMachine::GenericStateMachine<int> *m_parent_sm;
        TEngine::GameInputStateHandler *m_input_state_handler;
        TEngine::GameInputEventHandler *m_input_event_handler;
        SDL_Renderer *m_renderer;
    };
}