#include <SDL3/SDL_render.h>

#include "tetris_game_sm.hpp"

namespace TEngine::SM::States {
    void MenuState::enter() {
        tmp_counter = 5;
    }

    void MenuState::update(double delta_t) {
        tmp_counter -= delta_t;
        if (tmp_counter <= 0) {
            m_sm_ptr->switch_to("rungame");
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
                m_sm_ptr->switch_to("rungame");
            }
            m_input_event_handler->reset_events();
        }
    }

    // void MenuState::draw(SDL_Renderer *renderer) {
    //     SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    //     SDL_RenderClear(renderer);
    // }

    void MenuState::draw() {
        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);
    }

    void MenuState::reset(void) {
        tmp_counter = 0;
    }

    void MenuState::exit() {
        reset();
    }

}