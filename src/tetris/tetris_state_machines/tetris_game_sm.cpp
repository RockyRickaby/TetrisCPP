#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>

#include "tetris_game_sm.hpp"
#include "../../engine/text/text_renderer.hpp"

// TODO - have a working implementation of these that actually makes sense
namespace Tetris::States {
    void MenuState::enter() {
        tmp_counter = 5;
    }

    void MenuState::update(double delta_t) {
        tmp_counter -= delta_t;
        if (tmp_counter <= 0) {
            m_sm_ptr->switch_to(STATE_TETRIS);
        }
        m_menu->update(delta_t);
    }

    void MenuState::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        EventDispatcher ed{event};
        ed.dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev){ return OnKeyPressed(ev); });
    }


    void MenuState::draw() {
        // TEngine::Text::BitmapFontRenderer::draw_string_line(m_font, "MAIN MENU", 10, 10, 4);
        m_menu->draw();
    }

    void MenuState::reset(void) {
        tmp_counter = 0;
        m_menu->reset();
    }

    void MenuState::exit() {
        reset();
    }

    bool MenuState::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        // if (m_tetris_keys->keys.hard_drop.scancode == event.get_scancde()) {
        if (m_tetris_keys->any_match(event.get_scancde())) {
            m_sm_ptr->switch_to(STATE_TETRIS);
            return true;
        }
        return false;
    }

    void RunGameState::enter() {
        m_game_ptr->restart();
    }

    void RunGameState::update(double delta_t) {
        // TODO - everything from read_input_state could just be put here to make the handler
        // a bit more game-agnostic (would remove the need to include tetris_base.hpp in tetris_input.hpp)

        const auto read_input = [this]() {
            using namespace TEngine;
            using Tetrimino::Rotation;

            Vec2 dir{};
            Rotation rot{};
            // FIXME - if <- is currently pressed and -> is then pressed at the same time, -> should be prioritized over <-.
            // the opposite should also happen
            if (m_keyboard_state.may_press(m_tetris_keys->keys.left)) {
                dir = Vec2{-1,0};
            } else if (m_keyboard_state.may_press(m_tetris_keys->keys.right)) {
                dir = Vec2{1,0};
            } else if (m_keyboard_state.may_press(m_tetris_keys->keys.down)) {
                dir = Vec2{0,-1};
            }
            
            if (m_keyboard_state.may_press(m_tetris_keys->keys.rotate_clockwise)) {
                rot = Rotation::Clockwise;
            } else if (m_keyboard_state.may_press(m_tetris_keys->keys.rotate_counterclockwise)) {
                rot = Rotation::Counterclockwise;
            }

            return std::make_tuple(dir, rot);
        };

        auto [move, rot] = read_input();
        m_game_ptr->move(move);
        m_game_ptr->rotate(rot);

        if (!m_game_ptr->update(delta_t)) {
            m_parent_sm->switch_to(STATE_GAMEOVER);
        }
    }

    void RunGameState::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        EventDispatcher ed{event};
        ed.dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev){ return OnKeyPressed(ev); });
    }

    void RunGameState::draw() {
        m_game_ptr->draw();
    }

    void RunGameState::reset(void) {
        m_game_ptr->restart();
    }

    void RunGameState::exit() {
        reset();
    }

    bool RunGameState::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        if (m_tetris_keys->keys.hard_drop.scancode == event.get_scancde()) {
            m_game_ptr->do_hard_drop();
            return true;
        } else if (m_tetris_keys->keys.hold_piece.scancode == event.get_scancde()) {
            m_game_ptr->do_hold_piece();
            return true;
        } else if (m_tetris_keys->keys.pause.scancode == event.get_scancde()) {
            // m_game_ptr->do_pause();
            return true;
        }
        return false;
    }

    void GameOverState::enter(void) {

    }
    void GameOverState::update(double delta_t) {
        tmp_counter -= delta_t;
        if (tmp_counter <= 0) {
            m_sm_ptr->switch_to(STATE_MAIN_MENU);
        }
    }
    // void GameOverState::handle_input(void) {
    //     // TODO - noise.... seriously think of better ways to handle inputs (maybe try out implementing Events)
    //     // m_input_event_handler->reset_events();
    // }
    void GameOverState::draw(void) {
        TEngine::Text::BitmapFontRenderer::draw_string_line(m_font, "GAME OVER", 10, 10, 4);
    }
    void GameOverState::reset(void) {
        tmp_counter = 5;
    }
    void GameOverState::exit(void) {
        reset();
    }

    TetrisStateMachine::TetrisStateMachine(
        Tetris::Game* game_ptr,
        Tetris::MainMenu* menu_ptr, 
        SDL_Renderer* renderer,
        Tetris::Keybinds* tetris_keys,
        TEngine::Text::BitmapFont* font
    ) :
        m_runstate{this, game_ptr, tetris_keys, renderer},
        m_menustate{this, menu_ptr, tetris_keys, renderer, font},
        m_gameoverstate(this, renderer, font),
        m_states{ &m_menustate, &m_runstate, &m_gameoverstate },
        m_current{m_states.at(STATE_MAIN_MENU)},
        m_switching(false),
        m_next{-1}
    {}

    void TetrisStateMachine::update(double delta) {
        update_state();
        m_current->update(delta);
    }

    void TetrisStateMachine::switch_to(int id) {
        if (!m_current) {
            m_current = &m_menustate;
            m_current->enter();
        } else {
            m_switching = true;
            m_next = id;
        }
    }

    void TetrisStateMachine::update_state() {
        if (m_switching) {
            // m_curr->reset();
            m_current->exit();
            m_current = m_states.at(m_next);
            m_current->enter();
            // m_next.clear();
            m_switching = false;
        }
    }
}