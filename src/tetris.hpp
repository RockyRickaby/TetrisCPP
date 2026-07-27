#pragma once

#include <unordered_map>
#include <array>
#include <algorithm>

#include "tetris_base.hpp"
#include "tetris_bags.hpp"
#include "tetris_engine.hpp"
// #include "tetris_utils.hpp" // rotstr_to_int

namespace Tetris {
    // Implemented in kicks.cpp
    namespace __WallkickData {
        // some of the kick tests are the exact same for some rotations
        // only contains non-zero tests (no test Vec2 is equal to {0,0})
        extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_ANY;
        // some of the kick tests are the exact same for some rotations
        // only contains non-zero tests (no test is equal to {0,0})
        extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_I;
    }

    // matrix grows UPWARDS and to the right. must be flipped when rendered
    // note: not trivially copyable (duh)
    // note2: fully defined in header because template
    template<TetriminoQueue Queue, int ROWS = 22, int COLUMNS = 10, int BUFFER = 5>
    class Game {
    public:
        bool init() {
            // m_playfield_rtexture = playfield_rendertexture;
            m_started = true;
            place_and_spawn_piece();
            m_playfield_matrix.fill(Color{0,0,0,0});
            m_line_block_count.fill(0);
            return true;
        }

        void restart(void) {
            m_playfield_matrix.fill(Color{0,0,0,0});
            m_line_block_count.fill(0);
            place_and_spawn_piece();
            m_game_over = false;
            m_pieces_bag.reset();
            reset_lock();
        }
        // what to do:
        // - update grid when piece is locked
        bool update(double delta_t) {
            if (!m_started) {
                return false;
            }
            update_currentpiece(delta_t);
            // TODO - handle the gameover state better
            if (m_game_over) {
                restart();
                return true;
            } else if (std::find(m_line_block_count.begin(), m_line_block_count.end(), COLUMNS) != m_line_block_count.end()) {
                // if game is not over, and full lines are available to clean, then update the playfield
                update_playfield(delta_t);
            }
            update_ghostpiece();
            return true;
        }
        void move(Vec2 dir) {
            // no upwards movement allowed
            if (dir.y > 0) {
                dir.y = 0;
            }
            m_nextdir = dir;
        }
        void rotate(Tetrimino::Rotation r) {
            // if (m_nextrot == Tetrimino::Rotation::NONE) {
                m_nextrot = r;
            // }
        }

        void do_hard_drop(void) {
            m_hard_drop = true;
        }

        void do_hold_piece(void) {
            m_hold_piece = true;
        }
        // drawing logic is vertically mirrored
        // the playfield grows upwards, but the drawing grows downwards, so we flip it
        void draw(SDL_Renderer *renderer) {
            // SDL_SetRenderTarget(renderer, m_playfield_rtexture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);
            // // debug block
            // {
                // Vec2 min = m_current.get_min() + m_current.get_position(), max = m_current.get_max() + m_current.get_position();
                // SDL_FRect r_min{(min.x + m_board_offset_x) * m_scale, (min.y + m_board_offset_x) * m_scale, m_scale, m_scale};
                // SDL_FRect r_max{(max.x + m_board_offset_x) * m_scale, (max.y + m_board_offset_x) * m_scale, m_scale, m_scale};
                // std::cout << min << ' ' << max << std::endl;

            //     SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            //     SDL_RenderFillRect(renderer, &r_min);
            //     SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            //     SDL_RenderFillRect(renderer, &r_max);
            // }

            // draws buffer area;
            // SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            // for (int i = ROWS - 2; i < ROWS + BUFFER; i++) {
            //     for (int j = 0; j < COLUMNS; j++) {
            //         // Block b = m_playfield_matrix[j + i * COLUMNS];
            //         SDL_FRect r{ .x = (j + m_board_offset_x) * m_scale, .y = (i + m_board_offset_x) * m_scale, .w = m_scale, .h = m_scale };
            //         // SDL_FRect r{ .x = (b.pos.x + m_board_offset_x) * m_scale, .y = (b.pos.y + m_board_offset_x) * m_scale, .w = m_scale, .h = m_scale };
            //         // SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, SDL_ALPHA_OPAQUE);
            //         SDL_RenderRect(renderer, &r);
            //     }
            // }
            auto draw_piece = [this, renderer](const Tetrimino::Piece& p, bool fill = true){
                Color c = p.get_color();
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
                for (auto [x, y] : p) {
                    // Vec2 v = (v.pos + k->get_position());
                    SDL_FRect r{ .x = (x + m_board_offset_x) * m_scale, .y = ((ROWS - 1) - y + m_board_offset_y) * m_scale, .w = m_scale, .h = m_scale };
                    if (fill) {
                        SDL_RenderFillRect(renderer, &r);
                    } else {
                        SDL_RenderRect(renderer, &r);
                    }
                }
            };
            draw_piece(m_current);
            draw_playfield_blocks(renderer);
            // playfield drawn on top of everything
            draw_playfield_grid(renderer);

            if (m_ghost.get_position() != m_current.get_position()) {
                draw_piece(m_ghost, false);
            }

            // SDL_RenderTexture(renderer, m_playfield_rtexture, nullptr, nullptr);
            m_pieces_bag.draw(renderer, 0, 0, 0);
            // SDL_SetRenderTarget(renderer, nullptr);
        }
    private:
        enum class GameState {
            UNINITIALIZED,
            ONGOING,
            PAUSE,
            ANIMATION,
        };
        // TODO - organize this
        inline static constexpr Vec2 __invalid_move_vec = { 0xFFFF, 0xFFFF };
        std::array<Color, (ROWS + BUFFER) * COLUMNS> m_playfield_matrix = {}; // +5 rows of extra buffer
        std::array<int, ROWS + BUFFER> m_line_block_count = {};

        Queue m_pieces_bag{};
        Tetrimino::Piece m_current{};
        Tetrimino::Piece m_ghost{};
        Tetrimino::Piece m_held{};
        Tetrimino::Rotation m_nextrot = Tetrimino::Rotation::NONE;
        Vec2 m_nextdir = __invalid_move_vec;

        GameState m_state;
        bool m_started = true;
        bool m_hard_drop = false;
        bool m_hold_piece = false;
        bool m_game_over = false;
        float m_board_offset_x = 6;
        float m_board_offset_y = 1;
        float m_scale = 20;
        Engine::Countdown m_piece_drop_timer = Engine::Countdown{1, true};

        // extended placement lock down variables
        // can also be used for infinite placement lock down or classic lock down if moves is ignored
        struct {
            Engine::Countdown timer = Engine::Countdown{0.5, true};
            int moves = 15;

            void reset() {
                timer.reset();
                moves = 15;
            }
        } m_piece_lock;

        // SDL_Texture *m_playfield_rtexture = nullptr;
        // SDL_Texture *m_bag_texture = nullptr;

        void reset_lock() {
            m_piece_lock.reset();
        }
        
        bool update_currentpiece(double delta_t) {
            if (m_hold_piece) {
                // TODO - implement hold logic
            }
            if (m_hard_drop) {
                perform_hard_drop(m_current);
                place_and_spawn_piece();
                m_hard_drop = false;
                return true;
            }
            bool rotated = false;
            if (m_nextrot != Tetrimino::Rotation::NONE) {
                m_current.rotate(m_nextrot);
                if (!perform_wallkick(m_current)) {
                    m_current.rotate(Tetrimino::get_opposite_rotation(m_nextrot));
                } else {
                    // ROTATION WAS SUCCESSFUL!!
                    m_ghost.rotate(m_nextrot);
                    reset_lock();
                    rotated = true;
                }
                m_nextrot = Tetrimino::Rotation::NONE;
            }

            bool moved = false;
            bool moved_down = false;
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
            Vec2 m_min = m_current.get_min() + m_current.get_position();

            bool is_downwards_obstructed = false;
            // move down for a bit to see if there's anything below the piece
            m_current.move(Vec2{0,-1});
            if (check_collision(m_current)) {
                is_downwards_obstructed = true; // there is!!!!
            }
            // undo test move
            m_current.move(Vec2{0,1});

            if (is_downwards_obstructed) {
                // to have infinite placement lock down, just remove the lock.moves check
                if ((moved || rotated) && m_piece_lock.moves > 0) {
                    m_piece_lock.moves -= 1;
                    m_piece_lock.timer.reset();
                    // reset_lock();
                } else if (m_piece_lock.timer.done(delta_t)) {
                    place_and_spawn_piece();
                }
            } else {
                reset_lock();
                if (m_piece_drop_timer.done(delta_t)) { // this timer will auto-reset itself (and by the place_and_spawn_piece() function as well)
                    if (m_min.y > 0 && !moved_down) {
                        m_current.move(Vec2{ 0, -1 });
                        if (check_collision(m_current)) {
                            m_current.move(Vec2{ 0, 1 });
                            place_and_spawn_piece();
                        }
                    } else if (m_min.y <= 0) {
                        place_and_spawn_piece();
                    }
                }
            }

            return true;
        }

        // move ghost piece down until we hit something
        void update_ghostpiece(void) {
            m_ghost.move_position(m_current.get_position());
            perform_hard_drop(m_ghost);
        }

        bool place_and_spawn_piece(void) {
            for (Vec2 v : m_current) {
                m_playfield_matrix[v.x + v.y * COLUMNS] = m_current.get_color();
                m_line_block_count[v.y] += 1;
            }
            m_current = m_pieces_bag();
            if (check_collision(m_current)) {
                m_game_over = true;
                m_current = {};
                return false;
            }
            m_ghost = m_current;
            m_current.move(Vec2{0,-1});
            if (check_collision(m_current)) {
                m_current.move(Vec2{0,1});
            }
            m_piece_drop_timer.reset();
            m_piece_lock.timer.reset();
            return true;
        }

        // TODO - TEMPORARY WORKING SOLUTION. HAS NO SUPPORT AT ALL FOR ANY SORT OF ANIMATION, AS EVERYTHINNG IS EFFECTIVELY DONE IN ONE GO
        void update_playfield(double delta_t) {
            std::array<std::pair<int, int>, ROWS + BUFFER> ranges_to_remove;
            int count = 0;
            int curr = 0;
            int prev = 0;
            int rows_to_clean_n = std::count(m_line_block_count.begin(), m_line_block_count.end(), 10);
            // calculates which ranges have to be removed. range is [n,m), n = inclusive, m = exclusive
            // this loop also deletes the rows, which prevents implementing animations !!! for now
            // start from the bottom
            for (int i = 0; i < ROWS + BUFFER && rows_to_clean_n > 0; ++i) {
                if (m_line_block_count[i] == 10) {
                    rows_to_clean_n--;
                    std::fill(
                        m_playfield_matrix.begin() + (i * COLUMNS),
                        m_playfield_matrix.begin() + ((i * COLUMNS + COLUMNS)),
                        Color{}
                    );
                    m_line_block_count[i] = 0;
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
            ranges_to_remove[count] = std::make_pair(prev, curr);
            count++;
            for (int k = count - 1; k >= 0; --k) {
                std::pair<int, int>& range = ranges_to_remove[k];
                std::move(
                    m_playfield_matrix.begin() + (range.second * COLUMNS),
                    m_playfield_matrix.end(),
                    m_playfield_matrix.begin() + (range.first * COLUMNS)
                );
                // TODO -  consider way to clean only the places that actually had blocks instead of everything
                std::fill(
                    m_playfield_matrix.end() - (range.second * COLUMNS),
                    m_playfield_matrix.end(),
                    Color{}
                );
                std::move(m_line_block_count.begin() + range.second, m_line_block_count.end(), m_line_block_count.begin() + range.first);
                std::fill(m_line_block_count.end() - range.second, m_line_block_count.end(), 0);
            }
        }

        bool check_within_bounds(const Tetrimino::Piece &p) {
            Vec2 vmin = p.get_min() + p.get_position();
            Vec2 vmax = p.get_max() + p.get_position();
            return vmin.x >= 0 && vmin.y >= 0 && vmax.x < COLUMNS;
        }

        bool check_collision(const Tetrimino::Piece &p) {
            bool collides = !check_within_bounds(p);
            // TODO - consider better alternative for detecting existing blocks other than testing for alpha == 0
            for (auto it = p.begin(); it != p.end() && !collides; ++it) {
                Vec2 v = *it;
                collides = m_playfield_matrix[v.x + v.y * COLUMNS].a != 0;
            }
            return collides;
        }

        void perform_hard_drop(Tetrimino::Piece &p) {
            while (!check_collision(p)) {
                p.move(Vec2{0,-1});
            }
            // we hit something... move up to unstuck
            p.move(Vec2{0,1});
        }

        bool perform_wallkick(Tetrimino::Piece &p) {
            if (!check_collision(p)) {
                return true;
            }
            // no allocations
            const std::array<Vec2, 4> *kicks = nullptr;
            // the I piece has different wallkick tests
            if (p.get_type() == Tetrimino::Type::I) {
                kicks = &__WallkickData::WALLKICKS_I.at(p.get_rotation_state());
            } else {
                kicks = &__WallkickData::WALLKICKS_ANY.at(p.get_rotation_state());
            }
            for (Vec2 kick : *kicks) {
                p.move(kick);
                if (check_collision(p)) {
                    p.move(kick * -1); // undo kick if it's not successful
                } else {
                    return true;
                }
            }
            return false;
        }

        void draw_playfield_blocks(SDL_Renderer *renderer) {
            {SDL_FRect fr = {m_board_offset_x * m_scale, m_board_offset_y * m_scale, m_scale, m_scale};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &fr);
            fr.x = 0;
            fr.y = 0;
            SDL_RenderFillRect(renderer, &fr);
            fr.x = 630;
            fr.y = 470;
            SDL_RenderFillRect(renderer, &fr);}

            for (int i = 0; i < ROWS; i++) {
                for (int j = 0; j < COLUMNS; j++) {
                    Color c = m_playfield_matrix[j + i * COLUMNS];
                    if (c.a != 0) {
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
                        SDL_FRect r = {(j + m_board_offset_x) * m_scale, ((ROWS - 1) - i + m_board_offset_y) * m_scale, m_scale, m_scale};
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
        }

        void draw_playfield_grid(SDL_Renderer *renderer) {
            SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
            for (int i = 1; i < COLUMNS; i++) {
                SDL_RenderLine(renderer,
                    (i + m_board_offset_x) * m_scale,
                    (ROWS + m_board_offset_y) * m_scale,
                    (i + m_board_offset_x) * m_scale,
                    ((m_board_offset_y + 4) * m_scale) - (2 * m_scale)
                );
            }

            for (int i = 1; i < ROWS - 1; i++) {
                SDL_RenderLine(renderer,
                    m_board_offset_x * m_scale,
                    (ROWS - i + m_board_offset_y) * m_scale,
                    (COLUMNS + m_board_offset_x) * m_scale,
                    (ROWS - i + m_board_offset_y) * m_scale
                );
            }

            float thick = m_scale < 10 ? 1 : 4;
            float extra_spacing_top = 1; // [0,1], the smaller, the bigger the extra space
            // FIXME - fix rendering of lines when offset is different from 1
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, SDL_ALPHA_OPAQUE);
            SDL_FRect vertical1 = {
                m_board_offset_x * m_scale - thick,
                (m_board_offset_y + 2) * m_scale,
                thick,
                (ROWS - 1) * m_scale - (extra_spacing_top * m_scale)
            };
            SDL_RenderFillRect(renderer, &vertical1);
            SDL_FRect vertical2 = {
                (m_board_offset_x + COLUMNS) * m_scale,
                vertical1.y,
                thick,
                vertical1.h
            };
            SDL_RenderFillRect(renderer, &vertical2);
            SDL_FRect horizontal1 = {
                m_board_offset_x * m_scale - thick,
                (m_board_offset_y + 2) * m_scale - thick,
                (COLUMNS) * m_scale + thick * 2,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal1);
            SDL_FRect horizontal2 = {
                horizontal1.x,
                (m_board_offset_y + ROWS - 1 + 2) * m_scale - (extra_spacing_top * m_scale),
                horizontal1.w,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal2);
        }
    };
}