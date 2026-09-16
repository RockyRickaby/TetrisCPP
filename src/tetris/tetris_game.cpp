#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_render.h>
#include <memory>
#include <algorithm>

#include "tetris_game.hpp"
#include "../engine/text/text_renderer.hpp"
#include "../engine/utils.hpp"

// this may not be good practice, but whatever
using namespace TEngine; // Vec2, Color, Text

namespace Tetris {
    Game::Game(SDL_Renderer* renderer, int screen_width, int screen_height, float block_scale, Text::BitmapFont* font, SDL_Texture* mino_texture) :
        m_renderer{renderer},
        m_scale{block_scale},
        m_board_offset_x{(screen_width - COLUMNS * block_scale) / 2.0f},
        m_board_offset_y{(screen_height - ROWS * block_scale) / 2.0f},
        m_queue_offset_x{m_board_offset_x + (COLUMNS + 1.07f) * m_scale},
        m_queue_offset_y{m_board_offset_y + 6 * m_scale},
        m_text_font{font},
        m_mino_texture{mino_texture}
    {
        m_playfield_matrix.fill(Color{});
        m_line_block_count.fill(0);

        m_scoreboard.default_speed = m_piece_drop_timer.get_countdown_time();
        m_scoreboard.speed = m_piece_drop_timer.get_countdown_time();

        // set_piece_queue<Bag::OnePiece<Tetrimino::Type::O>>();
        set_piece_queue<Bag::Standard>();
        m_verts.reserve(4 * (ROWS - 2) * COLUMNS);
        m_indices.reserve(6 * (ROWS - 2) * COLUMNS);
        spawn_next_piece();
    }

    void Game::restart(void) {
        m_playfield_matrix.fill(Color{});
        m_line_block_count.fill(0);
        
        m_piece_drop_timer.reset();
        m_pieces_bag->reset();
        m_scoreboard.reset();
        m_piece_lock.reset();
        reset_piece_lock_timer();

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
    // the game must be restarted manually once the game is over
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
            m_update_state = GameUpdateState::GGameover;
            return false;
        };

        bool need_regen_playfield = false;
        // TODO - right now, the game will do nothing after the game is over. change this to something else later, idk
        switch (m_update_state) {
            case GameUpdateState::GPiece: {
                bool updated = update_fn(placed);
                if (!updated) {
                    return gameover_fn();
                }
                if (placed) {
                    need_regen_playfield = true;
                    if (std::find(m_line_block_count.begin(), m_line_block_count.end(), COLUMNS) != m_line_block_count.end()) {
                        if (!update_playfield(delta_t)) {
                            // if the animation is supposed to play, this branch is taken
                            m_update_state = GameUpdateState::GPlayfield;
                        }
                    } else if (m_piece_spawn.get_countdown_time() > 0) {
                        m_update_state = GameUpdateState::GSpawn;
                    } else if (!place_and_spawn_next_piece()) {
                        return gameover_fn();
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
                    }
                    // because the animation isn't "instantaneous", set countdown to almost done
                    // so that the piece spawns almost immediately after the playfield is updated,
                    // making the game a little bit more speedy
                    m_piece_spawn.done(m_piece_spawn.get_countdown_time() - 0.0001);
                }
            } break;

            case GameUpdateState::GSpawn: {
                if (m_piece_spawn.done(delta_t)) {
                    if (!spawn_next_piece()) {
                        return gameover_fn();
                    }
                    m_hold_recently_swapped = false;
                    m_update_state = GameUpdateState::GPiece;
                }
            } break;

            case GameUpdateState::GGameover: {
                // restart();
                return false;
            } break;

            default:
                std::clog << "Tetris::Game::update(double): reached default case somehow! oops\n";
                // std::abort();
                return false;
        }
        if (need_regen_playfield) {
            regen_playfield();
        }
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
        m_scoreboard.set_level(level);
        m_piece_drop_timer.set_countdown_time(m_scoreboard.speed);
    }

    int64_t Game::get_score(void) {
        return m_scoreboard.score;
    }

    void Game::draw() {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);
        draw_playfield_grid();
        draw_playfield_blocks();
        draw_piece(m_ghost, true, 128);
        draw_piece(m_current);

        float new_scale = m_scale / 1.5f;
        // using namespace Text;
        // 2.15
        // 1.75

        draw_held_piece(
            m_held,
            m_board_offset_x - 4.40f * m_scale,
            m_board_offset_y + 2 * m_scale,
            new_scale
        );
        m_pieces_bag->draw();
        draw_text_elements();
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
            perform_hard_drop(m_current);
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
        // NOTE - issues with this part may be caused by the input handling that happens outside of this class
        if (is_downwards_obstructed) {
            m_piece_lock.prev_y = m_current.get_position().y + m_current.get_min().y;
            // to have infinite placement lock down, just remove the lock.moves check
            if ((moved || rotated) && m_piece_lock.moves > 0) {
                m_piece_lock.moves -= 1;
                m_piece_lock.timer.reset();
                // std::cout << m_piece_lock.moves << std::endl;
                // reset_piece_lock_timer();
            } else if (m_piece_lock.timer.done(delta_t)) {
                // std::cout << "locking\n";
                return placed = place_current_piece();
            }
        } else {
            if (moved_down && m_current.get_position().y + m_current.get_min().y < m_piece_lock.prev_y) {
                reset_piece_lock_timer();
            }
            
            // this timer will auto-reset itself (and by the place_and_spawn_next_piece() function as well)
            if (m_piece_drop_timer.done(delta_t)) {
                if (m_min.y > 0 && !moved_down) {
                    m_current.move(Vec2{ 0, -1 });
                    reset_piece_lock_timer();
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
        const auto scoreboard_callback = [this](int cleared){
            m_scoreboard.update(cleared);
            m_piece_drop_timer.set_countdown_time(m_scoreboard.speed);
        };
        return m_update_playfield_routine(this, delta_t, scoreboard_callback);
    }

    bool Game::spawn_next_piece(void) {
        m_current = (*m_pieces_bag)(); 
        // piece spawns right on top (inside) another -> game over condition
        if (check_collision(m_current)) {
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

    // safe to call if the current piece is empty
    bool Game::place_current_piece(void) {
        // piece placed above the playfield -> game over condition
        if ((m_current.get_min() + m_current.get_position()).y >= ROWS - 2) {
            // m_game_over = true;
            m_current = {};
            return false;
        }
        for (Vec2 v : m_current) {
            size_t x = static_cast<size_t>(v.x);
            size_t y = static_cast<size_t>(v.y);
            m_playfield_matrix[x + y * COLUMNS] = m_current.get_color();
            m_line_block_count[y] += 1;
        }
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
            collides = TEngine::TUtils::color_to_int32(m_playfield_matrix[x + y * COLUMNS]) != 0;
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

    void Game::perform_hard_drop(Tetrimino::Piece &p) {
        while (!check_collision(p)) {
            p.move(Vec2{0,-1});
        }
        // we hit something... move up to unstuck
        p.move(Vec2{0,1});
    }

    bool Game::perform_wallkick(Tetrimino::Piece &p) {
        // first test case - regular rotation with no kicks
        if (!check_collision(p)) {
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
        for (Vec2 kick : *kicks) {
            p.move(kick);
            if (check_collision(p)) {
                p.move(kick * -1); // undo kick if it's not successful
            } else {
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
            m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "NEXT",
            m_board_offset_x + (COLUMNS + 1.4f) * m_scale,
            m_board_offset_y + 4.785f * m_scale,
            m_scale / 11.5f
        );

        float score_x_offset = m_board_offset_x + (COLUMNS + 1) * m_scale;
        float score_y_offset = 2;
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "SCORE",
            score_x_offset,
            m_board_offset_y + score_y_offset * m_scale,
            m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_int64(
            m_text_font, m_scoreboard.score,
            score_x_offset,
            m_board_offset_y + (score_y_offset + 1) * m_scale,
            m_scale / 11.5f
        );

        float level_x_offset = 4.5f;
        float level_y_offset = 7.2f;
        Text::BitmapFontRenderer::draw_string_line(
            m_text_font, "LEVEL",
            m_board_offset_x - level_x_offset * m_scale,
            m_board_offset_y + level_y_offset * m_scale,
            m_scale / 11.5f
        );
        Text::BitmapFontRenderer::draw_int64(
            m_text_font, m_scoreboard.level,
            m_board_offset_x - level_x_offset * m_scale,
            m_board_offset_y + (level_y_offset + 1) * m_scale,
            m_scale / 11.5f
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
                game_ptr->m_line_block_count[i] = 0;
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
            m_cleared += (snd - fst);
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
        ranges_to_remove[count++] = std::make_pair(prev, curr);
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
}