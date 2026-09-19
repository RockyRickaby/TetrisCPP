#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <array>

#include "tetris_game_sm.hpp"
#include "../../engine/text/text_renderer.hpp"

// just messing around with macros
// https://en.wikipedia.org/wiki/X_macro
#define STRING_LITERALS \
    X(__STR_HIGHSCORE, "new highscore") \
    X(__STR_GAMEOVER, "game over") \
    X(__STR_PAUSE, "pause") \
    X(__STR_GO, "go") \
    X(__STR_LEADERBOARD, "leaderboard") \
    X(__STR_RANK, "rank") \
    X(__STR_PLAYER, "player") \
    X(__STR_SCORE, "score") 
#define X(id, str) static constexpr std::string_view id = str;
STRING_LITERALS
#undef X
#undef STRING_LITERALS

static constexpr std::array __RANKS_ORDINAL = {
    "1st",
    "2nd",
    "3rd",
    "4th",
    "5th",
    "6th",
    "7th",
    "8th",
    "9th",
    "10th",
};

// no need for macros
// static constexpr std::string_view __STR_HIGHSCORE = "new highscore";
// static constexpr std::string_view __STR_GAMEOVER = "game over";
// static constexpr std::string_view __STR_PAUSE = "pause";
// static constexpr std::string_view __STR_GO = "go";
// static constexpr std::string_view __STR_LEADERBOARD = "leaderboard";

static size_t int64_len(std::int64_t num) {
    std::array<char, 64> buffer{};
    // std::fill(std::begin(buffer), std::end(buffer), 0);
    std::to_chars_result res = std::to_chars(buffer.data(), buffer.data() + buffer.size(), num);
    if (res.ec == std::errc::value_too_large) {
        return 0;
    }
    std::size_t len = static_cast<std::size_t>(res.ptr - buffer.data());
    return len;
}

// TODO - have a working implementation of these that actually makes sense
namespace Tetris::States {
    void MenuState::enter() {
        reset();
        tmp_counter.time = 10;
        m_read_any = true;
    }

    void MenuState::update(double delta_t) {
        if (!m_menu->update(delta_t)) {
            m_sm_ptr->switch_to(STATE_TETRIS);
        }
        if (tmp_counter.done(delta_t)) {
            tmp_counter.time = 0;
        }
    }

    // TODO - forward events to MainMenu instance (state-changin is triggered by checking if m_wants_switch is set)
    void MenuState::event(TEngine::Events::IEvent& event) {
        // using namespace TEngine::Events;
        // EventDispatcher ed{event};
        // ed.dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev){ return OnKeyPressed(ev); });
        m_menu->event(event);
    }


    void MenuState::draw() {
        // TEngine::Text::BitmapFontRenderer::draw_string_line(m_font, "MAIN MENU", 10, 10, 4);
        m_menu->draw();
    }

    void MenuState::reset(void) {
        tmp_counter.time = 0;
        m_read_any = true;
        m_menu->reset();
    }

    void MenuState::exit() {
    }

    bool MenuState::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        // if (m_tetris_keys->keys.hold_piece.scancode == event.scancode) {
        m_menu->event(event);
        // if ((m_tetris_keys->any_match(event.scancode) && m_read_any) || m_tetris_keys->keys.hold_piece.scancode == event.scancode) {
        //     m_menu->event(event);
        //     // if (m_menu->wants_switch_state()) {
        //     //     m_sm_ptr->switch_to(STATE_TETRIS);
        //     // }
        //     m_read_any = false;
        // }
        return event.handled;
    }






    void RunGameState::enter() {
        reset();
    }

    void RunGameState::update(double delta_t) {
        if (!m_begin_countdown.done(delta_t)) {
            return;
        }
        if (m_pause) {
            m_pause_countdown.set_countdown_time(3);
            return;
        } else if (!m_pause_countdown.done(delta_t)) {
            return;
        }

        m_go_count.done(delta_t);

        const auto read_input = [this, delta_t]() {
            using namespace TEngine;
            using namespace TEngine::Input;
            using Tetrimino::Rotation;

            Vec2 dir{};
            Rotation rot{};
            if (Keyboard::may_press(m_tetris_keys->keys.left, delta_t)) {
                dir = Vec2{-1,0};
            } else if (Keyboard::may_press(m_tetris_keys->keys.right, delta_t)) {
                dir = Vec2{1,0};
            } else if (Keyboard::may_press(m_tetris_keys->keys.down, delta_t)) {
                dir = Vec2{0,-1};
            }
            
            if (Keyboard::may_press(m_tetris_keys->keys.rotate_clockwise, delta_t)) {
                rot = Rotation::Clockwise;
            } else if (Keyboard::may_press(m_tetris_keys->keys.rotate_counterclockwise, delta_t)) {
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
        float scale = 5;
        if (m_begin_countdown.get_time_left() > 0) {
            TEngine::Text::BitmapFontRenderer::draw_int64(m_font, static_cast<int>(m_begin_countdown.get_time_left()) + 1, (960 - scale * m_font->tile_size()) / 2.0f, (720 - scale * m_font->tile_size()) / 2.0f, scale);
        } else if (!m_go_count.done(0)) {
            // scale = 5;
            TEngine::Text::BitmapFontRenderer::draw_string_line(
                m_font,
                __STR_GO,
                (960 - __STR_GO.size() * m_font->tile_size() * scale) / 2.0f,
                (720 - m_font->tile_size() * scale) / 2.0f,
                scale
            );
        }

        if (m_pause) {
            // scale = 5;
            TEngine::Text::BitmapFontRenderer::draw_string_line(
                m_font,
                __STR_PAUSE,
                (960 - __STR_PAUSE.size() * m_font->tile_size() * scale) / 2.0f,
                (720 - m_font->tile_size() * scale) / 2.0f,
                scale
            );
        }
        if (m_pause_countdown.get_time_left() > 0 && !m_pause) {
            TEngine::Text::BitmapFontRenderer::draw_int64(m_font, static_cast<int>(m_pause_countdown.get_time_left()) + 1, (960 - scale * m_font->tile_size()) / 2.0f, (720 - scale * m_font->tile_size()) / 2.0f, scale);
        }
    }

    void RunGameState::reset(void) {
        m_game_ptr->restart();
        m_begin_countdown.reset();
        m_pause_countdown.set_countdown_time(0);
        m_pause = false;
        
        m_run_state = State::Begin;
        m_go_count.time = 1;
    }

    void RunGameState::exit() {
    }

    bool RunGameState::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        if (m_begin_countdown.get_time_left() > 0) {
            return false;
        }

        if (m_tetris_keys->keys.hard_drop.scancode == event.scancode && m_pause_countdown.get_time_left() <= 0) {
            m_game_ptr->do_hard_drop();
            return true;
        } else if (m_tetris_keys->keys.hold_piece.scancode == event.scancode && m_pause_countdown.get_time_left() <= 0) {
            m_game_ptr->do_hold_piece();
            return true;
        } else if (m_tetris_keys->keys.pause.scancode == event.scancode) {
            // m_game_ptr->do_pause();
            m_pause = !m_pause;
            m_go_count.time = 0; // prevent GO! from being rendered when the game is paused right after starting
            return true;
        }
        return false;
    }








    void GameOverState::enter(void) {
        m_leaderboard.load_scores(m_lb_storage);
        m_has_highscore = m_leaderboard.is_new_highscore(m_game_ptr->get_score());
        reset();
        // TODO - enable input from here OR after the leaderboard is shown
        // SDL_StartTextInput(SDL_GetWindowFromID(m_window_id));
    }
    void GameOverState::update(double delta_t) {
        if (m_gameover_timer.done(delta_t)) {
            // m_sm_ptr->switch_to(STATE_MAIN_MENU);
        }
        if (!m_highscore_delay.done(delta_t)) {
            return;
        }

        if (m_has_highscore) {
            if (!m_leaderboard_transition_delay.done(delta_t)) {
                return;
            }
            m_typing_name = m_player_name.length() < Score::ScoreEntry::player_name_max_length;
        } else {
            m_leaderboard_transition_delay.time = 0;
        }

        if (m_blackscreen_pos.x <= -10000000) {
            m_blackscreen_pos = {};
        } else if (m_quitting && m_blackscreen_pos.y >= 720) {
            m_blackscreen_pos.y = -720;
        } else if ((m_quitting && m_blackscreen_pos.y <= 0) || (!m_quitting && m_blackscreen_pos.y < 720)) {
            m_blackscreen_pos.y += 720 * 1.5f * delta_t;
        } else if (m_quitting && m_blackscreen_pos.y >= 0) {
            m_sm_ptr->switch_to(STATE_MAIN_MENU);
        }

        if (m_blocks_framerate.done(delta_t)) {
            // if (m_blackscreen_pos.y < 720) {
            //     m_blackscreen_pos.y += 720 *1 * m_blocks_framerate.get_countdown_time();
            // }
            for (auto& p : m_blocks_wireframe) {
                auto& ins = p.instance;
                ins.rotate_y(SDL_PI_F/8.0f * m_blocks_framerate.get_countdown_time());
            }
        }
    
        if (m_switch_block.done(delta_t)) {
            ++m_block;
            if (m_block == m_blocks_wireframe.rend()) {
                m_block = m_blocks_wireframe.rbegin();
            }
        }
    }
    void GameOverState::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        EventDispatcher ed{event};
        ed.dispatch<KeyPressedEvent>([this](KeyPressedEvent& key) -> bool {
            if (m_typing_name) {
                handle_scancode(key.scancode);
            } else if (!m_gameover_timer.done(0)) {
                return false;
            } else if (!m_highscore_delay.done(0)) {
                m_highscore_delay.time = 0;
            } else if (!m_leaderboard_transition_delay.done(0)) {
                m_leaderboard_transition_delay.time = 0;
            } else {
                // m_sm_ptr->switch_to(STATE_MAIN_MENU);
                m_quitting = true;
            }
            return true;
        });
        ed.dispatch<KeyRepeatEvent>([this](KeyRepeatEvent& key){
            if (m_typing_name) {
                handle_scancode(key.scancode);
                return true;
            }
            return false;
        });
    }
    void GameOverState::draw(void) {
        using namespace TEngine::Text;
        if (m_highscore_delay.time > 0 && m_leaderboard_transition_delay.time > 0) {
            BitmapFontRenderer::draw_string_line(m_font, __STR_GAMEOVER, m_gameover_text_pos.x, m_gameover_text_pos.y, 5);
        }

        if (m_has_highscore && m_highscore_delay.time == 0 && m_leaderboard_transition_delay.time > 0) {
            BitmapFontRenderer::draw_string_line(m_font, __STR_HIGHSCORE, m_newscore_text_pos.x, m_newscore_text_pos.y, 5);
        }
        
        float text_scale = 2.75f;
        float ranks_offset = 3;
        float players_offset = 4;
        float scores_offset = 15;
        if (m_leaderboard_transition_delay.time == 0) {
            SDL_FRect black = {
                m_blackscreen_pos.x,
                m_blackscreen_pos.y,
                960,
                720
            };
            m_block->draw_wireframe();
            float y_off = 0;
            int count = m_has_highscore ? 1 : 0;
            BitmapFontRenderer::draw_string_line(m_font, __STR_RANK, m_leaderboard_offset.x - ranks_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y - 2 * text_scale * 12, text_scale);
            BitmapFontRenderer::draw_string_line(m_font, __STR_PLAYER, m_leaderboard_offset.x + players_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y - 2 * text_scale * 12, text_scale);
            BitmapFontRenderer::draw_string_line(m_font, __STR_SCORE, m_leaderboard_offset.x + scores_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y - 2 * text_scale * 12, text_scale);
            size_t total_ranks = m_leaderboard.size();
            if (m_has_highscore && total_ranks < 10) {
                total_ranks += 1;
            }
            for (size_t i = 0; i < total_ranks; i++) {
                BitmapFontRenderer::draw_int64(m_font, i + 1, m_leaderboard_offset.x - ranks_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                BitmapFontRenderer::draw_char(m_font, '.', m_leaderboard_offset.x - ((i + 1) >= 10 ? ranks_offset - 2 : ranks_offset - 1) * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                // BitmapFontRenderer::draw_string_line(m_font, __RANKS_ORDINAL.at(i), m_leaderboard_offset.x - ranks_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                y_off += text_scale * 12;
            }
            y_off = 0;
            const auto draws = [this, text_scale, players_offset, scores_offset, &y_off, &count](auto&& begin, auto&& end){
                for (auto it = begin; it != end && count < 10; ++it) {
                    const auto& [score, name] = *it;
                    BitmapFontRenderer::draw_string_line(m_font, name, m_leaderboard_offset.x + players_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                    BitmapFontRenderer::draw_int64(m_font, score, m_leaderboard_offset.x + scores_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                
                    y_off += text_scale * 12;
                    count++;
                }
            };
            BitmapFontRenderer::draw_string_line(m_font,
                __STR_LEADERBOARD,
                (960 - ((__STR_LEADERBOARD.size()) * m_font->tile_size() * text_scale)) / 2.0f,
                m_leaderboard_offset.y - 4.5f  * text_scale * 12,
                text_scale);
            if (m_has_highscore) {
                auto part = std::upper_bound(m_leaderboard.begin(), m_leaderboard.end(), m_game_ptr->get_score(),
                    [](const auto lhs, const auto& rhs){
                        return lhs > std::get<0>(rhs);
                    }
                );
                draws(m_leaderboard.begin(), part);
                BitmapFontRenderer::draw_string_line(m_font, m_player_name, m_leaderboard_offset.x + players_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                BitmapFontRenderer::draw_int64(m_font, m_game_ptr->get_score(), m_leaderboard_offset.x + scores_offset * m_font->tile_size() * text_scale, m_leaderboard_offset.y + y_off, text_scale);
                // TODO - draw a cursor instead of blinking and changing colors
                if (m_curr_ch) {
                    SDL_FRect text_cursor = {
                        m_leaderboard_offset.x + (m_player_name.length() + players_offset) * text_scale * m_font->tile_size(),
                        m_leaderboard_offset.y + y_off + text_scale * m_font->tile_size(),
                        text_scale * (m_font->tile_size() - 1),
                        text_scale
                    };
                    if (m_switch_block.get_time_left() >= 0.75f) {
                        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                        SDL_RenderFillRect(m_renderer, &text_cursor);
                    }
                    BitmapFontRenderer::draw_char(m_font, m_curr_ch, m_leaderboard_offset.x + (m_player_name.length() + players_offset) * text_scale * m_font->tile_size(), m_leaderboard_offset.y + y_off, text_scale);
                }
                y_off += text_scale * 12;
                draws(part, m_leaderboard.end());
            } else {
                draws(m_leaderboard.begin(), m_leaderboard.end());
            }
            SDL_SetRenderDrawColor(m_renderer, 0,0,0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(m_renderer, &black);
        }
    }

    void GameOverState::reset(void) {
        m_gameover_timer.time = 3;
        m_highscore_delay.time = m_has_highscore ? 3 : 5;
        m_leaderboard_transition_delay.time = 4;
        m_gameover_text_pos = { (960 - (5 * m_font->tile_size() * __STR_GAMEOVER.size())) / 2.0f, (720 - 5 * m_font->tile_size()) / 2.0f };
        m_newscore_text_pos = { (960 - (5 * m_font->tile_size() * __STR_HIGHSCORE.size())) / 2.0f, (720 - 5 * m_font->tile_size()) / 2.0f };
        
        std::int64_t highscore = m_leaderboard.top_score();
        if (m_has_highscore && m_game_ptr->get_score() > highscore) {
            highscore = m_game_ptr->get_score();
        }
        size_t len = int64_len(highscore);
        auto tmp = m_leaderboard.size();
        int extra_off = m_has_highscore ? tmp + 1 : tmp;
        extra_off = std::max(10, extra_off);

        float text_scale = 2.75f;
        float ranks_offset = 3;
        float scores_offset = 15;
        m_leaderboard_offset = {
            (960 - (((scores_offset - ranks_offset) * m_font->tile_size() * text_scale) + len * m_font->tile_size() * text_scale)) / 2.0f,
            (720 - ((extra_off - 3) * text_scale * 12)) / 2.0f
        };
        m_block = m_blocks_wireframe.rbegin();

        m_blocks_framerate.reset();
        m_switch_block.reset();

        m_player_name = "";
        m_curr_ch = 'a';
        m_typing_name = false;
        m_quitting = false;

        m_blackscreen_pos = {-10000000,-10000000};
    }

    void GameOverState::exit(void) {
        if (m_has_highscore) {
            m_leaderboard.push_score(m_game_ptr->get_score(), m_player_name);
        }
        m_leaderboard.flush_scores(m_lb_storage);
        // TODO - disable text input here
        // SDL_StopTextInput(SDL_GetWindowFromID(m_window_id));
    }

    void GameOverState::handle_scancode(SDL_Scancode code) {
        bool handle_return = false;
        char tmp = m_curr_ch;
        switch (code) {
            case SDL_SCANCODE_DOWN: {
                m_curr_ch = (m_curr_ch - 'a' - 1 + 26) % 26 + 'a';
            } break;
            case SDL_SCANCODE_UP: {
                m_curr_ch = (m_curr_ch - 'a' + 1 + 26) % 26 + 'a';
            } break;
            case SDL_SCANCODE_BACKSPACE: {
                if (!m_player_name.empty()) m_player_name.pop_back();
            } break;
            case SDL_SCANCODE_SPACE: {
                tmp = ' ';
                handle_return = true;
            } break;
            case SDL_SCANCODE_RETURN: {
                handle_return = true;
            } break;
            case SDL_SCANCODE_KP_ENTER: {
                handle_return = true;
            } break;
            
            default: break;
        }

        if (handle_return) {
            m_player_name += tmp;
            // m_curr_ch = !std::isalnum(m_curr_ch) ? 'a' : m_curr_ch;
            if (m_player_name.length() >= Score::ScoreEntry::player_name_max_length) {
                m_typing_name = false;
                m_curr_ch = 0;
            }
        }
    }








    
    TetrisStateMachine::TetrisStateMachine(
        Tetris::Game* game_ptr,
        Tetris::MainMenu* menu_ptr, 
        SDL_Renderer* renderer,
        Tetris::Keybinds* tetris_keys,
        TEngine::Text::BitmapFont* font,
        Score::Leaderboard& leaderboard,
        std::span<Tetris::Piece3D> blocks_wireframe,
        std::span<Tetris::Piece3D> blocks_filled,
        SDL_WindowID window_id
    ) :
        m_runstate{this, game_ptr, tetris_keys, font, renderer},
        m_menustate{this, game_ptr, menu_ptr, tetris_keys, renderer, font},
        m_gameoverstate(this, game_ptr, renderer, font, leaderboard, blocks_wireframe, window_id),
        m_states{ &m_menustate, &m_runstate, &m_gameoverstate },
        m_current{m_states.at(STATE_MAIN_MENU)},
        m_blocks_wireframe(blocks_wireframe),
        m_blocks_filled(blocks_filled),
        m_switching(false),
        m_next{-1}
    {
        m_current->enter();
    }

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