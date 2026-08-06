#pragma once

#include <SDL3/SDL_render.h>

#include "../../engine/state_machine.hpp"
#include "../../engine/tengine.hpp"
// #include "../../tetris.hpp"

namespace TEngine::SM::States {
    class MenuState final : public IDrawableState {
    public:
        MenuState(GenericStateMachine *sm, SDL_Renderer *renderer, GameInputEventHandler *input_event_h, GameInputStateHandler *input_state_h) :
            m_sm_ptr{sm},
            m_renderer{renderer},
            m_input_event_handler(input_event_h),
            m_input_state_handler(input_state_h)
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
        GenericStateMachine *m_sm_ptr;
        GameInputStateHandler *m_input_state_handler;
        GameInputEventHandler *m_input_event_handler;
        SDL_Renderer *m_renderer;
        double tmp_counter = 5;
    };

    template<typename Game>
    class RunGameState final : public IDrawableState {
    public:
        RunGameState(GenericStateMachine *sm, Game *game, SDL_Renderer *renderer, GameInputEventHandler *input_event_h, GameInputStateHandler *input_state_h) :
            m_game_ptr{game},
            m_parent_sm(sm),
            m_input_state_handler(input_state_h),
            m_input_event_handler(input_event_h),
            m_renderer{renderer}
        {}

        void enter() override {

        }

        void update(double delta_t) override {
            auto [move, rot] = m_input_state_handler->read_input_state();
            m_game_ptr->move(move);
            m_game_ptr->rotate(rot);
            if (!m_game_ptr->update(delta_t)) {
                // m_parent_sm->switch_to("gameover");
            }
        }

        // void handle_input_event(GameInputEventHandler& input_handler) override {
        //     if (input_handler.has_input_event) {
        //         if (input_handler.key.hard_drop.pressed) {
        //             m_game_ptr->do_hard_drop();
        //         } else if (input_handler.key.hold_piece.pressed) {
        //             m_game_ptr->do_hold_piece();
        //         } else if (input_handler.key.pause.pressed) {
        //             // m_game_ptr->do_pause();
        //         }
        //         input_handler.reset_events();
        //     }
        // }

        // void handle_input_state(GameInputStateHandler& input_handler) override {
        //     auto [move, rot] = input_handler.read_input_state();
        //     m_game_ptr->move(move);
        //     m_game_ptr->rotate(rot);
        // }

        void handle_input() override {
            if (m_input_event_handler->has_input_event) {
                if (m_input_event_handler->key.hard_drop.pressed) {
                    m_game_ptr->do_hard_drop();
                } else if (m_input_event_handler->key.hold_piece.pressed) {
                    m_game_ptr->do_hold_piece();
                } else if (m_input_event_handler->key.pause.pressed) {
                    // m_game_ptr->do_pause();
                }
                m_input_event_handler->reset_events();
            }
        }

        // void draw(SDL_Renderer *renderer) override {
        //     m_game_ptr->draw(renderer);
        // }

        void draw() override {
            m_game_ptr->draw(m_renderer);
        }

        void reset(void) override {
            m_game_ptr->restart();
        }

        void exit() override {
            reset();
        }

    private:
        Game *m_game_ptr;
        GenericStateMachine *m_parent_sm;
        GameInputStateHandler *m_input_state_handler;
        GameInputEventHandler *m_input_event_handler;
        SDL_Renderer *m_renderer;
    };
}