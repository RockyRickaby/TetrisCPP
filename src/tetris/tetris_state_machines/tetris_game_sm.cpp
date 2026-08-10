#include <SDL3/SDL_render.h>

#include "tetris_game_sm.hpp"

namespace Tetris::States {
    void MenuState::enter() {
        tmp_counter = 5;
    }

    void MenuState::update(double delta_t) {
        tmp_counter -= delta_t;
        if (tmp_counter <= 0) {
            m_sm_ptr->switch_to(STATE_TETRIS);
        }
    }

    // void MenuState::handle_input_event(GameInputEventHandler& input_handler) {
    //     if (input_handler.has_input_event) {
    //         if (input_handler.key.hard_drop.pressed) {
    //             m_sm_ptr->switch_to("rungame");
    //         }
    //         input_handler.reset_events();
    //     }
    // }

    // void MenuState::handle_input_state([[maybe_unused]] GameInputStateHandler& input_handler) {

    // }
    void MenuState::handle_input() {
        if (m_input_event_handler->has_input_event) {
            if (m_input_event_handler->key.hard_drop.pressed) {
                m_sm_ptr->switch_to(STATE_TETRIS);
            }
            m_input_event_handler->reset_events();
        }
    }

    // void MenuState::draw(SDL_Renderer *renderer) {
    //     SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    //     SDL_RenderClear(renderer);
    // }

    void MenuState::draw() {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);
    }

    void MenuState::reset(void) {
        tmp_counter = 0;
    }

    void MenuState::exit() {
        reset();
    }

    void RunGameState::enter() {

    }

    void RunGameState::update(double delta_t) {
        auto [move, rot] = m_input_state_handler->read_input_state();
        m_game_ptr->move(move);
        m_game_ptr->rotate(rot);
        if (!m_game_ptr->update(delta_t)) {
            // m_parent_sm->switch_to("gameover");
        }
    }

    void RunGameState::handle_input() {
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

    void RunGameState::draw() {
        m_game_ptr->draw(m_renderer);
    }

    void RunGameState::reset(void) {
        m_game_ptr->restart();
    }

    void RunGameState::exit() {
        reset();
    }
}