#include <SDL3/SDL_render.h>
#include <memory>
#include <print>
#include <ranges>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "tetris_game.hpp"
#include "../engine/text/text_renderer.hpp"
#include "../engine/utils.hpp"
#include "tetris_pieces.hpp"
#include "tetris_utils.hpp"

// this may not be good practice, but whatever
using namespace TEngine; // Vec2, Color, Text

namespace Tetris {
    Game::Game(
        SDL_Renderer* renderer,
        int screen_width,
        int screen_height,
        float block_scale,
        Text::BitmapFont* font,
        SDL_Texture* mino_texture,
        TEngine::Sound::Mixer* mixer
    ) :
        m_renderer{renderer},
        m_scale{block_scale},
        m_board_offset_x{(screen_width - COLUMNS * block_scale) / 2.0f},
        m_board_offset_y{(screen_height - ROWS * block_scale) / 2.0f},
        m_queue_offset_x{m_board_offset_x + (COLUMNS + 1.07f) * m_scale},
        m_queue_offset_y{m_board_offset_y + 6 * m_scale},
        m_mixer{mixer},
        m_text_font{font},
        m_mino_texture{mino_texture}
    {
        m_playfield_matrix.fill(Color{});
        m_line_block_count.fill(0);

        // m_scoreboard.default_speed = m_piece_drop_timer.get_countdown_time();
        // m_scoreboard.speed = m_piece_drop_timer.get_countdown_time();

        m_adv_scoreboard.__default_speed = m_piece_drop_timer.get_countdown_time();
        m_adv_scoreboard.speed = m_piece_drop_timer.get_countdown_time();
        m_scoreboard_events.reset(this);

        // set_piece_queue<Bag::OnePiece<Tetrimino::Type::O>>();
        set_piece_queue<Bag::Standard>();
        m_verts.reserve(4 * (ROWS - 2) * COLUMNS);
        m_indices.reserve(6 * (ROWS - 2) * COLUMNS);
        spawn_next_piece();

        m_update_playfield_gameover.black_screen_dimensions = {
            static_cast<float>(screen_width),
            static_cast<float>(screen_height)
        };
    }

    void Game::restart(void) {
        m_nextrot = Tetrimino::Rotation::None;
        m_hard_drop = false;
        m_hold_piece = false;
        m_nextdir = {};

        m_playfield_matrix.fill(Color{});
        m_line_block_count.fill(0);

        m_update_playfield_gameover.black_screen_pos = { -100000, -100000 };

        
        m_piece_drop_timer.reset();
        m_pieces_bag->reset();
        // m_scoreboard.reset();
        m_adv_scoreboard.reset();
        m_scoreboard_events.reset(this);
        m_piece_lock.reset();
        // m_piece_lock.reset();

        m_held = {};
        m_ghost = {};
        m_current = {};
        m_update_state = {};
        m_hold_recently_swapped = false;
        m_piece_drop_timer.set_countdown_time(1);
        spawn_next_piece();
        regen_playfield();
    }

    // returns false when the game is over.
    // returns true if the game can still go on.
    // the game must be restarted manually once its over
    bool Game::update(double delta_t) {
        bool placed = false;
        const auto update_fn = [this, delta_t](bool& placed) -> bool {
            if (update_currentpiece(delta_t, placed)) {
                update_ghostpiece();
                return true;
            }
            return false;
        };

        const auto gameover_fn = [this](){
            MIX_PlayTag(m_mixer->get(), "gameover", 0);
            m_update_state = GameUpdateState::GGameover;
            m_scoreboard_events.reset(this);
            return false;
        };

        const auto scoreboard_adv_fn = [this]{
            m_adv_scoreboard.update();
            m_piece_drop_timer.set_countdown_time(m_adv_scoreboard.speed);
        };

        bool need_regen_playfield = false;
        switch (m_update_state) {
            case GameUpdateState::GPiece: {
                bool updated = update_fn(placed);
                if (!updated) {
                    m_piece_lock.reset();
                    place_current_piece();
                    return gameover_fn();
                }
                if (placed) {
                    need_regen_playfield = true;
                    if (std::find(m_line_block_count.begin(), m_line_block_count.end(), COLUMNS) != m_line_block_count.end()) {
                        if (!update_playfield(delta_t)) {
                            // if the animation is supposed to play, this branch is taken
                            m_update_state = GameUpdateState::GPlayfield;
                            if (m_update_playfield_routine.m_cleared == 4) {
                                MIX_PlayTag(m_mixer->get(), "line_clear1", 0);
                            } else {
                                MIX_PlayTag(m_mixer->get(), "line_clear3", 0);
                            }
                        }
                    } else if (m_piece_spawn.get_countdown_time() > 0) {
                        m_update_state = GameUpdateState::GSpawn;
                        MIX_PlayTag(m_mixer->get(), "piece_lock", 0);
                    } else if (!place_and_spawn_next_piece()) {
                        return gameover_fn();
                    } else {
                        MIX_PlayTag(m_mixer->get(), "piece_lock", 0);
                        scoreboard_adv_fn();
                    }
                }
            } break;

            case GameUpdateState::GPlayfield: {
                need_regen_playfield = true;
                if (update_playfield(delta_t)) {
                    if (m_piece_spawn.get_countdown_time() > 0) {
                        m_update_state = GameUpdateState::GSpawn;
                    } else {
                        if (!place_and_spawn_next_piece()) {
                            return gameover_fn();
                        }
                        m_update_state = GameUpdateState::GPiece;
                        scoreboard_adv_fn();
                    }
                    // because the animation isn't "instantaneous", set countdown to almost done
                    // so that the piece spawns almost immediately after the playfield is updated,
                    // making the game a little bit more speedy
                    if (m_piece_spawn.get_countdown_time() > 0) {
                        m_piece_spawn.done(m_piece_spawn.get_countdown_time() - 0.0001);
                    }
                }
            } break;

            case GameUpdateState::GSpawn: {
                if (m_piece_spawn.done(delta_t)) {
                    if (!spawn_next_piece()) {
                        return gameover_fn();
                    }
                    scoreboard_adv_fn();
                    m_hold_recently_swapped = false;
                    m_update_state = GameUpdateState::GPiece;
                }
            } break;

            case GameUpdateState::GGameover: {
                // restart();
                if (m_update_playfield_gameover(this, delta_t)) {
                    need_regen_playfield = true;
                } else {
                    return false;
                }
            } break;

            default:
                std::clog << "Tetris::Game::update(double): reached default case somehow! oops\n";
                // std::abort();
                return false;
        }
        if (need_regen_playfield) {
            regen_playfield();
        }
        m_scoreboard_events.update(delta_t);
        return true;
    }

    void Game::move(Vec2 dir) {
        // no upwards movement allowed
        if (dir.y > 0) {
            dir.y = 0;
        }
        m_nextdir = dir;
    }

    void Game::set_level(int level) {
        // m_scoreboard.set_level(level);
        m_adv_scoreboard.set_level(level);
        // m_piece_drop_timer.set_countdown_time(m_scoreboard.speed);
        m_piece_drop_timer.set_countdown_time(m_adv_scoreboard.speed);
    }

    int64_t Game::get_score(void) {
        // return m_scoreboard.score;
        return m_adv_scoreboard.score;
    }

    void Game::draw() {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);
        draw_playfield_grid();
        draw_playfield_blocks();
        if (m_ghost.get_position() != m_current.get_position()) {
            draw_piece(m_ghost, true, 128);
        }
        draw_piece(m_current, true, static_cast<std::uint8_t>(m_piece_lock.piece_alpha * 255));

        float new_scale = m_scale / 1.5f;

        draw_held_piece(
            m_held,
            m_board_offset_x - 4.40f * m_scale,
            m_board_offset_y + 2 * m_scale,
            new_scale
        );
        m_pieces_bag->draw();
        draw_text_elements();

        m_update_playfield_gameover.draw(this);
        m_scoreboard_events.draw();
    }

    bool Game::try_move(bool &moved_down) {
        bool moved = false;
        moved_down = false;
        if (m_nextdir != __invalid_move_vec) {
            if (m_nextdir.x != 0) {
                m_nextdir.y = 0; // prioritize horizontal moves
            } 
            m_current.move(m_nextdir);
            if (check_collision(m_current)) {
                m_current.move(m_nextdir * -1); // we hit something. undo move
            } else {
                // m_min = m_current.get_min() + m_current.get_position();
                moved = true;
                if (m_nextdir.y < 0) {
                    m_piece_drop_timer.reset();
                    moved_down = true;
                    m_adv_scoreboard.push_action(ScoreSystemAdvanced::ScoreActions::SoftDrop);
                }
            }
            m_nextdir = __invalid_move_vec;
        }
        return moved;
    }

    bool Game::try_rotate(void) {
        bool rotated = false;
        if (m_nextrot != Tetrimino::Rotation::None) {
            m_current.rotate(m_nextrot);
            if (!perform_wallkick(m_current)) {
                m_current.rotate(Tetrimino::get_opposite_rotation(m_nextrot));
            } else {
                // ROTATION WAS SUCCESSFUL!!
                m_ghost.rotate(m_nextrot);
                rotated = true;
            }
            m_nextrot = Tetrimino::Rotation::None;
        }
        return rotated;
    }

    bool Game::update_currentpiece(double delta_t, bool& placed) {
        // no need for states here... it's too simple for all that
        if (m_current.get_type() == Tetrimino::Type::None) {
            return true;
        }
        if (m_hold_piece) {
            m_hold_piece = false;
            if (!m_hold_recently_swapped) {
                perform_hold();
                return true;
            }
        }
        if (m_hard_drop) {
            int lines_dropped = perform_hard_drop(m_current);
            while (lines_dropped) {
                m_adv_scoreboard.push_action(ScoreSystemAdvanced::ScoreActions::HardDrop);
                lines_dropped -= 1;
            }
            m_hard_drop = false;
            return placed = place_current_piece();
        }
        bool rotated = try_rotate();
        bool moved_down = false;
        bool moved = try_move(moved_down);
        Vec2 m_min = m_current.get_min() + m_current.get_position();
        // move down for a bit to see if there's anything below the piece
        m_current.move(Vec2{0,-1});
        bool is_downwards_obstructed = check_collision(m_current); // there might be...
        // undo test move
        m_current.move(Vec2{0,1});
        // m_piece_lock.piece_alpha = std::max(m_piece_lock.timer.get_time_left() * 2.0, 0.2);
        // NOTE - issues with this part may be caused by the input handling that happens outside of this class
        if (is_downwards_obstructed) {
            m_piece_lock.piece_alpha = TEngine::fmap_to_range(0.0f, 0.5f, 0.25f, 1.0f, m_piece_lock.timer.get_time_left());
            m_piece_lock.prev_y = m_current.get_position().y + m_current.get_min().y;
            // to have infinite placement lock down, just remove the lock.moves check
            if ((moved || rotated) && m_piece_lock.moves > 0) {
                m_piece_lock.moves -= 1;
                m_piece_lock.timer.reset();
                // std::cout << m_piece_lock.moves << std::endl;
                // m_piece_lock.reset();
            } else if (m_piece_lock.timer.done(delta_t)) {
                // std::cout << "locking\n";
                return placed = place_current_piece();
            }
        } else {
            if (moved_down && m_current.get_position().y + m_current.get_min().y < m_piece_lock.prev_y) {
                m_piece_lock.reset();
            }
            m_piece_lock.piece_alpha = 1;
            
            // this timer will auto-reset itself (and by the place_and_spawn_next_piece() function as well)
            if (m_piece_drop_timer.done(delta_t)) {
                if (m_min.y > 0 && !moved_down) {
                    m_current.move(Vec2{ 0, -1 });
                    m_piece_lock.reset();
                    if (check_collision(m_current)) {
                        m_current.move(Vec2{ 0, 1 });
                        return placed = place_current_piece();
                    }
                } else if (m_min.y <= 0) {
                    return placed = place_current_piece();
                }
            }
        }
        placed = false;
        return true;
    }

    void Game::update_ghostpiece(void) {
        if (m_ghost.get_type() != Tetrimino::Type::None) {
            m_ghost.move_position(m_current.get_position());
            perform_hard_drop(m_ghost);
        }
    }

    bool Game::update_playfield(double delta_t) {
        const auto scoreboard_callback = [this](int cleared, int highest_line){
            (void)highest_line;
            // m_scoreboard.update(cleared);
            // m_piece_drop_timer.set_countdown_time(m_scoreboard.speed);
            m_adv_scoreboard.push_action(static_cast<ScoreSystemAdvanced::ScoreActions>(1u << (cleared - 1)));
            // m_scoreboard_events.enable_texts_for(static_cast<ScoreSystemAdvanced::ScoreActions>(1u << (cleared - 1)));
        };
        return m_update_playfield_routine(this, delta_t, scoreboard_callback);
    }

    bool Game::spawn_next_piece(void) {
        m_current = (*m_pieces_bag)(); 
        // piece spawns right on top (inside) another -> game over condition
        if (check_collision(m_current)) {
            // TODO - //
            place_piece(m_current);
            // m_game_over = true;
            m_current = {};
            return false;
        }
        m_ghost = m_current;
        m_current.move(Vec2{0,-1}); // move one or so positions down (as required by the specs (I think?))
        if (check_collision(m_current)) {
            m_current.move(Vec2{0,1});
        }
        m_piece_drop_timer.reset();
        m_piece_lock.timer.reset();
        perform_hard_drop(m_ghost);
        return true;
    }

    void Game::place_piece(const Tetrimino::Piece& p) {
        for (Vec2 v : p) {
            size_t x = static_cast<size_t>(v.x);
            size_t y = static_cast<size_t>(v.y);
            m_playfield_matrix[x + y * COLUMNS] = p.get_color();
            m_line_block_count[y] += 1;
        }
    }

    // safe to call if the current piece is empty
    bool Game::place_current_piece(void) {
        // piece placed above the playfield -> game over condition
        if ((m_current.get_min() + m_current.get_position()).y >= ROWS - 2) {
            // m_game_over = true;
            place_piece(m_current);
            m_current = {};
            return false;
        }
        place_piece(m_current);
        // TODO - just a warning
        m_ghost = {};
        m_current = {};
        return true;
    }

    // safe to call if the current piece is empty
    bool Game::place_and_spawn_next_piece(void) {
        if (!place_current_piece()) {
            return false;
        }
        if (!spawn_next_piece()) {
            return false;
        }
        m_hold_recently_swapped = false;
        return true;
    }

    bool Game::check_within_bounds(const Tetrimino::Piece &p) {
        Vec2 vmin = p.get_min() + p.get_position();
        Vec2 vmax = p.get_max() + p.get_position();
        return vmin.x >= 0 && vmin.y >= 0 && vmax.x < COLUMNS;
    }

    bool Game::check_collision(const Tetrimino::Piece &p) {
        bool collides = !check_within_bounds(p);
        for (auto it = p.begin(); it != p.end() && !collides; ++it) {
            Vec2 v = *it;
            size_t x = static_cast<size_t>(v.x);
            size_t y = static_cast<size_t>(v.y);
            collides = m_playfield_matrix[x + y * COLUMNS] != Color{};
        }
        return collides;
    }

    bool Game::perform_hold() {
        if (!m_hold_recently_swapped) {
            bool needs_spawn = false;
            if (m_held.get_type() == Tetrimino::Type::None) {
                m_held = Tetrimino::get_piece(m_current.get_type());
                needs_spawn = true;
            } else {
                Tetrimino::Type tmp = m_held.get_type();
                m_held = Tetrimino::get_piece(m_current.get_type());
                m_current = Tetrimino::get_piece(tmp);
                needs_spawn = false;
            }
            if (!needs_spawn) {
                m_current.move_position(Tetrimino::get_piece(m_current.get_type()).get_position());
                m_current.move(Vec2{0,-1});
                if (check_collision(m_current)) {
                    m_current.move(Vec2{0,1});
                }
                m_ghost = m_current;
            } else {
                spawn_next_piece();
            }
            // m_current.move_position(Tetrimino::get_piece(m_current.get_type()).get_position()); // move to default position
            m_hold_recently_swapped = true;
            return needs_spawn;
        }
        return false;
    }

    int Game::perform_hard_drop(Tetrimino::Piece &p) {
        int lines_dropped = 0;
        while (!check_collision(p)) {
            p.move(Vec2{0,-1});
            lines_dropped += 1;
        }
        // we hit something... move up to unstuck
        p.move(Vec2{0,1});
        lines_dropped -= 1;
        return lines_dropped;
    }

    void Game::check_and_award_tspin(const Tetrimino::Piece &p, int rot_point) {
        if (p.get_type() != Tetrimino::Type::T) {
            return;
        }
        const auto pos = p.get_position();
        std::array<Vec2, 4> corners = {
            // top
            pos + Vec2{-1, 1}, // a
            pos + Vec2{1, 1}, // b
            // base
            pos + Vec2{1, -1}, // d
            pos + Vec2{-1, -1}, // c
        };

        int a = TetrisUtils::rotint_to_a_idx(p.get_rotation_state());
        int b = (a + 1) % 4;
        int d = (a + 2) % 4;
        int c = (a + 3) % 4;

        const auto check = [this](Vec2 p){
            return p.x < 0 || p.y < 0 || p.x >= COLUMNS || m_playfield_matrix[p.x + p.y * COLUMNS] != Color{};
        };
        bool is_full = (check(corners[a]) && check(corners[b])) && (check(corners[c]) || check(corners[d]));
        bool is_mini = (check(corners[c]) && check(corners[d])) && (check(corners[a]) || check(corners[b]));
        
        m_adv_scoreboard.erase_spin();
        if (is_full) {
            if (rot_point == 5) {
                m_adv_scoreboard.is_5rot_spin = true;
            }
            m_adv_scoreboard.push_action(ScoreSystemAdvanced::ScoreActions::TSpin);
            m_scoreboard_events.enable_texts_for(ScoreSystemAdvanced::ScoreActions::TSpin);
        } else if (is_mini) {
            if (rot_point == 5) {
                m_adv_scoreboard.is_5rot_spin = true;
                m_adv_scoreboard.push_action(ScoreSystemAdvanced::ScoreActions::TSpin);
                m_scoreboard_events.enable_texts_for(ScoreSystemAdvanced::ScoreActions::TSpin);
            } else {
                m_adv_scoreboard.push_action(ScoreSystemAdvanced::ScoreActions::MiniTSpin);
                m_scoreboard_events.enable_texts_for(ScoreSystemAdvanced::ScoreActions::MiniTSpin);
            }
        }
    }

    bool Game::perform_wallkick(Tetrimino::Piece &p) {
       // first test case - regular rotation with no kicks
        int rotation_point = 1;
        if (!check_collision(p)) {
            check_and_award_tspin(p, rotation_point);
            return true;
        }
        // no allocations
        const std::array<Vec2, 4> *kicks = nullptr;
        // the I piece has different wallkick tests
        if (p.get_type() == Tetrimino::Type::I) {
            kicks = &__Internal::WallkickData::WALLKICKS_I.at(p.get_rotation_state());
        } else {
            kicks = &__Internal::WallkickData::WALLKICKS_ANY.at(p.get_rotation_state());
        }
        // other kick tests are handled here
        rotation_point += 1;
        for (Vec2 kick : *kicks) {
            p.move(kick);
            if (check_collision(p)) {
                p.move(kick * -1); // undo kick if it's not successful
                rotation_point += 1;
            } else {
                check_and_award_tspin(p, rotation_point);
                return true;
            }
        }
        // there's no way for the current rotation to be successful at this point
        return false;
    }

    void Game::draw_piece(const Tetrimino::Piece& p, bool fill, Uint8 alpha) {
        Color c = p.get_color();
        SDL_BlendMode mode;
        SDL_GetRenderDrawBlendMode(m_renderer, &mode);
        if (alpha != SDL_ALPHA_OPAQUE) {
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        }
        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, alpha);

        Color text_color;
        SDL_GetTextureAlphaMod(m_mino_texture, &text_color.a);
        SDL_GetTextureColorMod(m_mino_texture, &text_color.r, &text_color.g, &text_color.b);
        SDL_SetTextureColorMod(m_mino_texture, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(m_mino_texture, alpha);
        for (auto [x, y] : p) {
            SDL_FRect rect = { .x = x * m_scale + m_board_offset_x, .y = ((ROWS - 1) - y) * m_scale + m_board_offset_y, .w = m_scale, .h = m_scale };
            if (fill) {
                SDL_RenderTexture(m_renderer, m_mino_texture, nullptr, &rect);
            } else {
                SDL_RenderRect(m_renderer, &rect);
            }
        }
        if (alpha != SDL_ALPHA_OPAQUE) {
            SDL_SetRenderDrawBlendMode(m_renderer, mode);
        }
        SDL_SetTextureColorMod(m_mino_texture, text_color.r, text_color.g, text_color.b);
        SDL_SetTextureAlphaMod(m_mino_texture, text_color.a);
    };

    void Game::draw_held_piece(const Tetrimino::Piece& p, float offset_x, float offset_y, float scale){
        Color c = p.get_color();

        std::array<SDL_FRect, 4> rects;
        float extra_off_x = 2;
        float extra_off_y = 3.25;
        const auto type = p.get_type();
        if (type == Tetrimino::Type::I) {
            extra_off_x = 0.55f;
            // extra_off_y = 3;
        } else if (type == Tetrimino::Type::O) {
            extra_off_x = 1.5f;
        }

        Color text_color;
        SDL_GetTextureAlphaMod(m_mino_texture, &text_color.a);
        SDL_GetTextureColorMod(m_mino_texture, &text_color.r, &text_color.g, &text_color.b);
        if (m_hold_recently_swapped) {
            SDL_SetTextureColorMod(m_mino_texture, 128, 128, 128);
        } else {
            SDL_SetTextureColorMod(m_mino_texture, c.r, c.g, c.b);
        }
        SDL_SetTextureAlphaMod(m_mino_texture, SDL_ALPHA_OPAQUE);
        for (auto [x, y] : p.get_blocks()) {
            SDL_FRect rect = { .x = (x + extra_off_x) * scale + offset_x, .y = offset_y - (y - extra_off_y) * scale, .w = scale, .h = scale };
            SDL_RenderTexture(m_renderer, m_mino_texture, nullptr, &rect);
        }
        // SDL_RenderFillRects(m_renderer, rects.data(), rects_i);
        SDL_SetTextureColorMod(m_mino_texture, text_color.r, text_color.g, text_color.b);
        SDL_SetTextureAlphaMod(m_mino_texture, text_color.a);
        
        float thick = 4;
        if (m_scale < 10) {
            thick = 1;
        }

        rects[0] = {
            offset_x - thick,
            offset_y - thick,
            thick,
            5 * scale + thick * 2
        };
        rects[1] = {
            offset_x + 5 * scale,
            rects[0].y,
            thick,
            rects[0].h
        };
        rects[2] = {
            offset_x,
            offset_y - thick,
            5 * scale,
            thick
        };
        rects[3] = {
            rects[2].x,
            offset_y + 5 * scale,
            rects[2].w,
            thick
        };
        Color border_color = TEngine::TUtils::color_from_hex("#808080");
        SDL_SetRenderDrawColor(m_renderer, border_color.r, border_color.g, border_color.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRects(m_renderer, rects.data(), 4);
    }

    void Game::draw_playfield_blocks() {
        SDL_RenderGeometry(
            m_renderer,
            m_mino_texture, m_verts.data(),
            static_cast<int>(m_verts.size()),
            m_indices.data(),
            static_cast<int>(m_indices.size())
        );

        // for (int i = 0; i < ROWS - 2; i++) {
        //     for (int j = 0; j < COLUMNS; j++) {
        //         Color c = m_playfield_matrix[j + i * COLUMNS];
        //         if (c.a != 0) {
        //             SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
        //             SDL_FRect r = {j * m_scale + m_board_offset_x, ((ROWS - 1) - i) * m_scale + m_board_offset_y, m_scale, m_scale};
        //             SDL_RenderFillRect(m_renderer, &r);
        //         }
        //     }
        // }
    }

    void Game::draw_playfield_grid() {
        SDL_SetRenderDrawColor(m_renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
        for (int i = 1; i < COLUMNS; i++) {
            SDL_RenderLine(m_renderer,
                i * m_scale + m_board_offset_x,
                ROWS * m_scale + m_board_offset_y,
                i * m_scale + m_board_offset_x,
                (4 * m_scale + m_board_offset_y) - (2 * m_scale)
            );
        }

        for (int i = 1; i < ROWS - 1; i++) {
            SDL_RenderLine(m_renderer,
                m_board_offset_x,
                (ROWS - i) * m_scale + m_board_offset_y,
                COLUMNS * m_scale + m_board_offset_x,
                (ROWS - i) * m_scale + m_board_offset_y
            );
        }

        float thick = 4;
        if (m_scale < 10) {
            thick = 1;
        }
        float extra_spacing_top = 1; // [0,1], the smaller, the bigger the extra space
        std::array<SDL_FRect, 4> rects;
        rects[0] = {
            m_board_offset_x - thick,
            2 * m_scale + m_board_offset_y,
            thick,
            (ROWS - 1) * m_scale - (extra_spacing_top * m_scale)
        };
        rects[1] = {
            COLUMNS * m_scale + m_board_offset_x,
            rects[0].y,
            thick,
            rects[0].h
        };
        rects[2] = {
            m_board_offset_x - thick,
            2 * m_scale - thick + m_board_offset_y,
            COLUMNS * m_scale + thick * 2,
            thick
        };
        rects[3] = {
            rects[2].x,
            ((ROWS - 1 + 2) * m_scale + m_board_offset_y) - (extra_spacing_top * m_scale),
            rects[2].w,
            thick
        };
        SDL_SetRenderDrawColor(m_renderer, 0x80, 0x80, 0x80, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRects(m_renderer, rects.data(), 4);
    }

    void Game::draw_text_elements() {
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "HOLD",
            m_board_offset_x - 4.05f * m_scale,
            m_board_offset_y + 2.40f * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "NEXT",
            m_board_offset_x + (COLUMNS + 1.4f) * m_scale,
            m_board_offset_y + 4.785f * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );

        float score_x_offset = m_board_offset_x + (COLUMNS + 1) * m_scale;
        float score_y_offset = 2;
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "SCORE",
            score_x_offset,
            m_board_offset_y + score_y_offset * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_int64(
            m_text_font, m_adv_scoreboard.score,
            score_x_offset,
            m_board_offset_y + (score_y_offset + 1) * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );

        float level_x_offset = 4.5f;
        float level_y_offset = 7.2f;
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "LEVEL",
            m_board_offset_x - level_x_offset * m_scale,
            m_board_offset_y + level_y_offset * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_int64(
            m_text_font, m_adv_scoreboard.level,
            m_board_offset_x - level_x_offset * m_scale,
            m_board_offset_y + (level_y_offset + 1) * m_scale,
            m_scale / 11.5f, m_scale / 11.5f
        );
    }
    
    void Game::regen_playfield(void) {
        // this might be overkill
        // will regenerate the entire thing on line-clears AND lock down
        m_verts.clear();
        m_indices.clear();
        int v_idx = 0;
        for (int i = 0; i < (ROWS + BUFFER) * COLUMNS; ++i) {
            Color c = m_playfield_matrix[i];
            if (c.a == 0) {
                continue;
            }
            SDL_FRect rect{
                .x = m_board_offset_x + static_cast<float>(i % COLUMNS) * m_scale,
                .y = m_board_offset_y + ((ROWS - 1) - (static_cast<float>(i / COLUMNS))) * m_scale,
                .w = m_scale,
                .h = m_scale
            };
            SDL_FColor color = {
                c.r / 255.f,
                c.g / 255.f,
                c.b / 255.f,
                c.a / 255.f
            };
            
            SDL_Vertex v0 = {
                { rect.x, rect.y},
                color,
                {0, 0}
            };
            SDL_Vertex v1 = {
                { rect.x + rect.w, rect.y},
                color,
                {1, 0}
            };
            SDL_Vertex v2 = {
                { rect.x + rect.w, rect.y + rect.h},
                color,
                {1, 1}
            };
            SDL_Vertex v3 = {
                { rect.x, rect.y + rect.h},
                color,
                {0, 1}
            };
        
            m_verts.emplace_back(v0);
            m_verts.emplace_back(v1);
            m_verts.emplace_back(v2);
            m_verts.emplace_back(v3);
            
            m_indices.emplace_back(v_idx + 0);
            m_indices.emplace_back(v_idx + 1);
            m_indices.emplace_back(v_idx + 2);
            
            m_indices.emplace_back(v_idx + 2);
            m_indices.emplace_back(v_idx + 3);
            m_indices.emplace_back(v_idx + 0);
            
            v_idx += 4;
        }
    }

    void Game::ScoreSystem::update(int cleared) {
        int multiplier = 100;
        if (cleared >= 2) multiplier += 200;
        if (cleared >= 3) multiplier += 200;
        if (cleared >= 4) multiplier += 300;
        score += multiplier * level;

        lines_cleared += cleared;
        if (level < max_level && lines_cleared >= goal) {
            level = level + 1;
            level = level > max_level ? max_level : level;
            speed = calculate_speed();
            goal = 10;
            lines_cleared = 0;
        }
    }

    void Game::ScoreSystem::reset(void) {
        score = 0;
        level = 1;
        lines_cleared = 0;
        speed = default_speed;
    }

    void Game::ScoreSystem::set_level(int new_level) {
        if (new_level > 1) {
            goal = new_level * lines_to_clear_per_level_multiplier;
            level = new_level;
            speed = calculate_speed();
        } else {
            goal = 10;
        }
    }

    double Game::ScoreSystem::calculate_speed() {
        double res = 1;
        for (int exp = level - 1; exp > 0; --exp) {
            res *= (0.8 - ((level - 1) * 0.007));
        }
        return res;
    }


    void Game::ScoreSystemAdvanced::erase_spin(void) {
        if (!is_5rot_spin) {    
            constexpr unsigned int tspins_mask =
                static_cast<unsigned int>(ScoreActions::MiniTSpin) |
                static_cast<unsigned int>(ScoreActions::TSpin);
            __performed_actions &= ~(tspins_mask);
        }
    }

    void Game::ScoreSystemAdvanced::push_action(ScoreActions action) {
        using enum ScoreActions;
        
        const unsigned int act = static_cast<unsigned int>(action);
        constexpr unsigned int line_clear_mask =
            static_cast<unsigned int>(Single) |
            static_cast<unsigned int>(Double) |
            static_cast<unsigned int>(Triple) |
            static_cast<unsigned int>(Tetris);
        constexpr unsigned int tspins_mask =
            static_cast<unsigned int>(MiniTSpin) |
            static_cast<unsigned int>(TSpin);
        // have we already set a line clear action? or perhaps a spin action?
        if (
            (act & line_clear_mask && __performed_actions & line_clear_mask) ||
            (act & tspins_mask && __performed_actions & tspins_mask)
        ) {
            return;
        }
        switch(action) {
            case None: break;
            case SoftDrop: __soft_lines += 1; break;
            case HardDrop: __hard_lines += 1; break;
            default: __performed_actions |= act; break;
        }
    }

    void Game::ScoreSystemAdvanced::update(void) {
        int score_to_add = 0;
        int cleared = 0;
        switch(static_cast<ScoreActions>(__performed_actions)) {
            using enum ScoreActions;
            case Single: {
                cleared = 1;
                score_to_add = 100;
                __is_back_to_back = false;
            } break;
            case Double: {
                cleared = 2;
                score_to_add = 300;
                __is_back_to_back = false;
            } break;
            case Triple: {
                cleared = 3;
                score_to_add = 500;
                __is_back_to_back = false;
            } break;
            case Tetris: {
                cleared = 4;
                score_to_add = 800;
            } break;
            case MiniTSpin: score_to_add = 100; break;
            case TSpin: score_to_add = 400; break;
            case MiniTSpinSingle: score_to_add = 200; break;
            case TSpinSingle: score_to_add = 800; break;
            case TSpinDouble: score_to_add = 1200; break;
            case TSpinTriple: score_to_add = 1600; break;
            default:
                break;
        }
        // TODO - implement back to back bonuses
        // if (__is_back_to_back) {
        //     score_to_add += (score_to_add / 2);
        // }
        __performed_actions = 0;

        score += score_to_add * level;
        score += __soft_lines;
        score += __hard_lines * 2;

        __soft_lines = 0;
        __hard_lines = 0;

        __lines_cleared += cleared;
        if (level < max_level && __lines_cleared >= __goal) {
            level = level + 1;
            level = level > max_level ? max_level : level;
            speed = calculate_speed();
            __goal = 10;
            __lines_cleared = 0;
        }

        is_5rot_spin = false;
    }

    void Game::ScoreSystemAdvanced::reset(void) {
        score = 0;
        level = 1;
        __lines_cleared = 0;
        speed = __default_speed;
        is_5rot_spin = false;

        __soft_lines = 0;
        __hard_lines = 0;
        __performed_actions = static_cast<unsigned int>(ScoreActions::None);
        __is_back_to_back = false;
    }

    void Game::ScoreSystemAdvanced::set_level(int new_level) {
        if (new_level > 1) {
            __goal = new_level * __lines_to_clear_per_level_multiplier;
            level = new_level;
            speed = calculate_speed();
        } else {
            __goal = 10;
        }
    }

    double Game::ScoreSystemAdvanced::calculate_speed(void) {
        double res = 1;
        for (int exp = level - 1; exp > 0; --exp) {
            res *= (0.8 - ((level - 1) * 0.007));
        }
        return res;
    }

    void Game::ScoreEventsTextManager::reset(Game* game_ptr) {
        ongoing_texts.clear();
        using namespace TEngine::Text;
        m_game_ptr = game_ptr;
        if (!init) {
            events_text[0] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "Single")};
            events_text[1] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "Double")};
            events_text[2] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "Triple")};
            events_text[3] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "Tetris")};
            events_text[4] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "MiniTSpin")};
            events_text[5] = {BitmapFontRenderer::make_bitmap_text_str(game_ptr->m_text_font, "TSpin")};

            init = true;
        }
    }

    void Game::ScoreEventsTextManager::enable_texts_for(ScoreSystemAdvanced::ScoreActions action, int height) {
        int idx = 0;
        using enum ScoreSystemAdvanced::ScoreActions;
        switch (action) {
            // holy moly
            case Single: idx = 0; break;
            case Double: idx = 1; break;
            case Triple: idx = 2; break;
            case Tetris: idx = 3; break;
            case MiniTSpin: idx = 4; break;
            case TSpin: idx = 5; break;
            default:
                return;
        }
        // move ~tres textos de altura para cima
        auto& text = events_text[idx];
        Vec2 pos{};
        if (action == MiniTSpin || action == TSpin) {
            auto y_aux = (ROWS - 4) - m_game_ptr->m_current.get_position().y;
            auto x_aux = m_game_ptr->m_current.get_position().x;
            
            pos = (Vec2{x_aux, y_aux} * m_game_ptr->m_scale)
                - Vec2{(text.scale_width * 2) / 2.0f, 0} 
                + Vec2{m_game_ptr->m_board_offset_x, m_game_ptr->m_board_offset_y};
        } else {
            pos = Vec2{
                -4.25f,
                std::max(
                    (ROWS - 2) - (static_cast<float>(height - 1)),
                    static_cast<float>(ROWS - 10)
                )
            } * m_game_ptr->m_scale + Vec2{m_game_ptr->m_board_offset_x, m_game_ptr->m_board_offset_y};
        }
        ongoing_texts.emplace_back(Countdown{2, false}, pos, idx, 15);
    }

    void Game::ScoreEventsTextManager::update(double delta_t) {
        for (auto& text : ongoing_texts) {
            text.visible_duration.done(delta_t);
            text.position.y -= text.time_to_speed() * delta_t;
        }

        // auto it = ongoing_texts.end();
        auto to_remove_begin = std::remove_if(
            ongoing_texts.begin(),
            ongoing_texts.end(),
            [](EventText& ev_text){ return ev_text.visible_duration.done(0); }
        );
        ongoing_texts.erase(to_remove_begin, ongoing_texts.end());
    }

    int Game::ScoreEventsTextManager::EventText::time_to_speed() {
        static constexpr float start_speed = 60.0f;
        static constexpr std::array m_text_speeds = {
            1.00f * start_speed,
            0.97f * start_speed,
            0.87f * start_speed,
            0.74f * start_speed,
            0.58f * start_speed,
            0.42f * start_speed,
            0.26f * start_speed,
            0.13f * start_speed,
            0.04f * start_speed,
            // 0.00f * start_speed,
        };
        float f = fmap_to_range(
            0,
            visible_duration.get_countdown_time(),
            0,
            static_cast<float>(m_text_speeds.size() - 1),
            visible_duration.get_countdown_time() - visible_duration.get_time_left()
        );
        return m_text_speeds.at(static_cast<int>(f));
    }

    void Game::ScoreEventsTextManager::draw() {
        for (auto& text_ev : ongoing_texts) {
            if (!text_ev.visible_duration.done(0)) {
                float threshold = 1.0f;
                if (text_ev.visible_duration.get_time_left() <= threshold) {
                    SDL_SetTextureAlphaMod(
                        events_text[text_ev.text_idx].text_data.get(),
                        static_cast<uint8_t>(TEngine::fmap_to_range(
                            0,
                            threshold,
                            0,
                            1,
                            text_ev.visible_duration.get_time_left()
                        ) * 255)
                    );
                } else {
                    SDL_SetTextureAlphaMod(events_text[text_ev.text_idx].text_data.get(), SDL_ALPHA_OPAQUE);
                }

                TEngine::Text::draw_bitmap_text(
                    events_text[text_ev.text_idx],
                    text_ev.position.x,
                    text_ev.position.y,
                    2,
                    4
                );
            }
        }
    }




    void Game::PieceLockdownTimer::reset(void) {
        timer.reset();
        moves = 15;
        prev_y = -1;
        piece_alpha = 1.0f;
    }

    void Game::PlayfieldUpdateRoutineAnimated::reset() {
        m_clearing_r = COLUMNS / 2;
        m_clearing_l = COLUMNS % 2 == 0 ? (COLUMNS / 2) - 1 : COLUMNS / 2;
        m_ranges_to_remove.clear();
        m_cleared = 0;
    }

    void Game::PlayfieldUpdateRoutineAnimated::get_ranges(Game *game_ptr) {
        constexpr int expected_max_ranges = 2;
        m_ranges_to_remove.reserve(expected_max_ranges);
        int curr = 0;
        int prev = 0;
        ptrdiff_t rows_to_clean_n = std::count(game_ptr->m_line_block_count.begin(), game_ptr->m_line_block_count.end(), COLUMNS);
        for (int i = 0; i < ROWS + BUFFER && static_cast<int>(rows_to_clean_n) > 0; ++i) {
            if (game_ptr->m_line_block_count[i] == 10) {
                rows_to_clean_n--;
                // TODO - remember this..
                // game_ptr->m_line_block_count[i] = 0;
                curr++;
            } else {
                if (curr - prev > 0) {
                    m_ranges_to_remove.emplace_back(prev, curr);
                }
                prev = i + 1;
                curr = i + 1;
            }
        }
        m_ranges_to_remove.emplace_back(prev, curr);
    }

    void Game::PlayfieldUpdateRoutineAnimated::pull_down(Game *game_ptr) {
        for (auto it = m_ranges_to_remove.rbegin(); it != m_ranges_to_remove.rend(); ++it) {
            const auto [fst, snd] = *it;
            // TODO - remember this... this is now calculated at the start of the routine
            // m_cleared += (snd - fst);
            std::move(
                game_ptr->m_playfield_matrix.begin() + (snd * COLUMNS),
                game_ptr->m_playfield_matrix.end(),
                game_ptr->m_playfield_matrix.begin() + (fst * COLUMNS)
            );
            std::fill(
                game_ptr->m_playfield_matrix.end() - ((snd - fst) * COLUMNS),
                game_ptr->m_playfield_matrix.end(),
                Color{}
            );
            std::move(game_ptr->m_line_block_count.begin() + snd, game_ptr->m_line_block_count.end(), game_ptr->m_line_block_count.begin() + fst);
            std::fill(game_ptr->m_line_block_count.end() - (snd - fst), game_ptr->m_line_block_count.end(), 0);
        }                
    }

    int Game::PlayfieldUpdateRoutineInstant::get_ranges(Game *game_ptr, std::array<std::pair<int, int>, 4>& ranges_to_remove) {
        // std::array<std::pair<int, int>, 4> ranges_to_remove;
        int count = 0;
        int curr = 0;
        int prev = 0;
        ptrdiff_t rows_to_clean_n = std::count(game_ptr->m_line_block_count.begin(), game_ptr->m_line_block_count.end(), COLUMNS);
        // calculates which ranges have to be removed. range is [n,m), n = inclusive, m = exclusive
        // this loop also deletes the rows, which prevents implementing animations !!! for now
        // start from the bottom
        for (int i = 0; i < ROWS + BUFFER && static_cast<int>(rows_to_clean_n) > 0; ++i) {
            if (game_ptr->m_line_block_count[i] == 10) {
                rows_to_clean_n--;
                std::fill(
                    game_ptr->m_playfield_matrix.begin() + (i * COLUMNS),
                    game_ptr->m_playfield_matrix.begin() + ((i * COLUMNS + COLUMNS)),
                    Color{}
                );
                game_ptr->m_line_block_count[i] = 0;
                curr++;
            } else {
                if (curr - prev > 0) {
                    ranges_to_remove[count] = std::make_pair(prev, curr);
                    count++;
                }
                prev = i + 1;
                curr = i + 1;
            }
        }
        // if we implement animations, this will still run at the very end of it
        // also, use this to calculate the points
        ranges_to_remove[count] = std::make_pair(prev, curr);
        count++;
        return count;
    }

    int Game::PlayfieldUpdateRoutineInstant::pull_rows(Game* game_ptr, std::array<std::pair<int, int>, 4>& ranges_to_remove, int size) {
        int cleared = 0;
        for (int k = size - 1; k >= 0; --k) {
            // std::pair<int, int>& range = ranges_to_remove[k];
            const auto [fst, snd] = ranges_to_remove[k];
            cleared += (snd - fst);
            std::move(
                game_ptr->m_playfield_matrix.begin() + (snd * COLUMNS),
                game_ptr->m_playfield_matrix.end(),
                game_ptr->m_playfield_matrix.begin() + (fst * COLUMNS)
            );
            std::fill(
                game_ptr->m_playfield_matrix.end() - ((snd - fst) * COLUMNS),
                game_ptr->m_playfield_matrix.end(),
                Color{}
            );
            std::move(game_ptr->m_line_block_count.begin() + snd, game_ptr->m_line_block_count.end(), game_ptr->m_line_block_count.begin() + fst);
            std::fill(game_ptr->m_line_block_count.end() - (snd - fst), game_ptr->m_line_block_count.end(), 0);
        }
        return cleared;
    }

    bool Game::PlayfieldUpdateRoutineGameOver::operator()(Game* game_ptr, double delta_t) {
        if (!initial_delay.done(delta_t)) {
            return true;
        }
        if (move_black_screen) {
            if (black_screen_pos.y < -black_screen_dimensions.y) {
                black_screen_pos = {0, -black_screen_dimensions.y};
            }
            black_screen_pos.y += black_screen_dimensions.y * 2.0f * delta_t;
            if (black_screen_pos.y >= 0) {
                move_black_screen = false;
                initial_delay.time = 1.40;
                row = 0;
                return false;
            }
            return true;
        } else if (animation_timer.done(delta_t)) { // this timer auto-resets
            TEngine::Color fill = {};
            // this routine auto-resets..
            if (row >= ROWS + BUFFER) {
                // animation_timer.reset();
                move_black_screen = true;
                return true;
            }
            if (row < ROWS - 2) {
                fill = TEngine::TUtils::color_from_hex("#808080");
            }
            std::fill(
                game_ptr->m_playfield_matrix.begin() + row * COLUMNS,
                game_ptr->m_playfield_matrix.begin() + COLUMNS + row * COLUMNS,
                fill);
            row++;
        }
        return true;
    }

    void Game::PlayfieldUpdateRoutineGameOver::draw(Game* game_ptr) {
        SDL_FRect r {
            .x = black_screen_pos.x,
            .y = black_screen_pos.y,
            .w = black_screen_dimensions.x,
            .h = black_screen_dimensions.y
        };
        SDL_SetRenderDrawColor(game_ptr->m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(game_ptr->m_renderer, &r);
    }
}