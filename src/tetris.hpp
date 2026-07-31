#pragma once

#include <unordered_map>
#include <array>
#include <algorithm>

#include "tetris_base.hpp"
#include "tetris_bags.hpp"
#include "tetris_engine.hpp"
// #include "tetris_utils.hpp" // rotstr_to_int

namespace Tetris {
    namespace __Internal {
        // Implemented in kicks.cpp
        namespace WallkickData {
            // some of the kick tests are the exact same for some rotations
            // only contains non-zero tests (no test Vec2 is equal to {0,0})
            extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_ANY;
            // some of the kick tests are the exact same for some rotations
            // only contains non-zero tests (no test is equal to {0,0})
            extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_I;
        }
    };

    // TODO - give this class a nice constructor to initialize some of the members more nicely
    // matrix grows UPWARDS and to the right. must be flipped when rendered
    // note: not trivially copyable (duh)
    // note2: fully defined in header because template
    template<TetriminoQueue Queue, int ROWS = 22, int COLUMNS = 10, int BUFFER = 5>
    class Game {
    public:
        bool init(int screen_width, int screen_height, float block_scale) {
            // m_playfield_rtexture = playfield_rendertexture;
            m_scale = block_scale;
            m_board_offset_x = (screen_width - COLUMNS * block_scale) / 2.0f;
            m_board_offset_y = (screen_height - ROWS * block_scale) / 2.0f;

            m_started = true;
            spawn_next_piece();
            m_playfield_matrix.fill(Color{});
            m_line_block_count.fill(0);
            update_playfield_routine.game_ptr = this; // TODO - dangerous... think of better options
            return true;
        }

        void restart(void) {
            m_playfield_matrix.fill(Color{});
            m_line_block_count.fill(0);
            m_held = {};
            m_current = {};
            m_game_over = false;
            m_pieces_bag.reset();
            reset_lock();
            scoreboard.reset();
            m_piece_drop_timer.reset();
            m_piece_drop_timer.m_time_delta = 1;
            spawn_next_piece();
        }
        // what to do:
        // - update grid when piece is locked
        bool update(double delta_t) {
            if (!m_started) {
                return false;
            }
            if (m_updating_playfield) {
                m_updating_playfield = !update_playfield_routine(delta_t, [this](int cleared){
                    scoreboard.update(cleared);
                    m_piece_drop_timer.m_time_delta = scoreboard.speed;
                });
                if (m_updating_playfield) {
                    return true;
                }
            }
            if (!m_updating_playfield) {
                update_currentpiece(delta_t);
            }
            // TODO - implement "perform_gameover" whatever (function, functor, idk)
            if (m_game_over) {
                restart();
                // m_game_over = true;
                return false;
            }
            if (std::find(m_line_block_count.begin(), m_line_block_count.end(), COLUMNS) != m_line_block_count.end()) {
                // this specific call will just "prepare" the routine, collecting the ranges with blocks to clear
                m_updating_playfield = !update_playfield_routine(delta_t, []<typename... F>(F... args){});
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
            m_nextrot = r;
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
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);
            // if (m_ghost.get_position() != m_current.get_position()) {
                draw_piece(renderer, m_ghost, true, 128);
            // }
            draw_piece(renderer, m_current);
            draw_playfield_blocks(renderer);
            draw_playfield_grid(renderer);

            float new_scale = m_scale / 1.5f;
            draw_held_piece(renderer, m_held, m_board_offset_x - 4.40 * m_scale, m_board_offset_y + 2 * m_scale, new_scale);
            m_pieces_bag.draw(renderer, m_board_offset_x + (COLUMNS + 1) * m_scale, m_board_offset_y + 6 * m_scale, new_scale);
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
        std::array<std::uint8_t, ROWS + BUFFER> m_line_block_count = {};

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
        bool m_updating_playfield = false;

        bool m_hold_recently_swapped = false;
        // these 3 floats will be changed in the init method 
        float m_scale = 20;
        float m_board_offset_x = ((32 - COLUMNS) / 2.0f) * m_scale;
        float m_board_offset_y = 1 * m_scale;
        Engine::Countdown m_piece_drop_timer = Engine::Countdown{1, true};

        // extended placement lock down variables
        // can also be used for infinite placement lock down or classic lock down if moves is ignored
        struct __PieceLockdownTimer {
            Engine::Countdown timer = Engine::Countdown{0.5, true};
            int moves = 15;

            void reset(void) {
                timer.reset();
                moves = 15;
            }
        } m_piece_lock;

        struct __ScoreSystem {
            uint64_t score = 0;
            int level = 1;
            int max_level = 15;
            int lines_to_clear_per_level_multiplier = 5;
            int lines_cleared = 0;
            double speed = 1;

            void update(int cleared) {
                int multiplier = 100;
                if (cleared >= 2) multiplier += 200;
                if (cleared >= 3) multiplier += 200;
                if (cleared >= 4) multiplier += 300;
                score += multiplier * level;

                lines_cleared += cleared;
                if (level < max_level && lines_cleared >= level * lines_to_clear_per_level_multiplier) {
                    level  = (level + 1) % max_level;
                    double res = 1;
                    for (int exp = level - 1; exp > 0; --exp) {
                        res *= (0.8 - ((level - 1) * 0.007));
                    }
                    speed = res;
                }
            }
            
            void reset(void) {
                score = 0;
                level = 1;
                max_level = 15;
                lines_to_clear_per_level_multiplier = 5;
                lines_cleared = 0;
                speed = 1;
            }
        } scoreboard;

        void reset_lock() {
            m_piece_lock.reset();
        }

        bool try_move(bool &moved_down) {
            bool moved = false;
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

        bool try_rotate(void) {
            bool rotated = false;
            if (m_nextrot != Tetrimino::Rotation::NONE) {
                m_current.rotate(m_nextrot);
                if (!perform_wallkick(m_current)) {
                    m_current.rotate(Tetrimino::get_opposite_rotation(m_nextrot));
                } else {
                    // ROTATION WAS SUCCESSFUL!!
                    m_ghost.rotate(m_nextrot);
                    rotated = true;
                }
                m_nextrot = Tetrimino::Rotation::NONE;
            }
            return rotated;
        }
        
        bool update_currentpiece(double delta_t) {
            if (m_hold_piece) {
                m_hold_piece = false;
                if (!m_hold_recently_swapped) {
                    perform_hold();
                    return true;
                }
            }
            if (m_hard_drop) {
                perform_hard_drop(m_current);
                place_and_spawn_next_piece();
                m_hard_drop = false;
                return true;
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

            if (is_downwards_obstructed) {
                // to have infinite placement lock down, just remove the lock.moves check
                if ((moved || rotated) && m_piece_lock.moves > 0) {
                    m_piece_lock.moves -= 1;
                    m_piece_lock.timer.reset();
                    // reset_lock();
                } else if (m_piece_lock.timer.done(delta_t)) {
                    place_and_spawn_next_piece();
                }
            } else {
                // FIXME - LOCK SHOULD ONLY BE RESET ON ROW CHANGE (I.E. IF THE PIECE MOVES UP (by wallkicking) OR DOWN)
                reset_lock();
                if (m_piece_drop_timer.done(delta_t)) { // this timer will auto-reset itself (and by the place_and_spawn_next_piece() function as well)
                    if (m_min.y > 0 && !moved_down) {
                        m_current.move(Vec2{ 0, -1 });
                        if (check_collision(m_current)) {
                            m_current.move(Vec2{ 0, 1 });
                            place_and_spawn_next_piece();
                        }
                    } else if (m_min.y <= 0) {
                        place_and_spawn_next_piece();
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

        bool spawn_next_piece(void) {
            m_current = m_pieces_bag();
            // piece spawns right on top (not necessarily above) another -> game over condition
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

        bool place_and_spawn_next_piece(void) {
            // piece placed above the playfield -> game over condition
            if ((m_current.get_min() + m_current.get_position()).y >= ROWS - 2) {
                m_game_over = true;
                m_current = {};
                return false;
            }
            for (Vec2 v : m_current) {
                m_playfield_matrix[v.x + v.y * COLUMNS] = m_current.get_color();
                m_line_block_count[v.y] += 1;
            }
            spawn_next_piece();
            m_hold_recently_swapped = false;
            return true;
        }

        class __PlayfieldUpdateRoutine{
        public:
            Game<Queue, ROWS, COLUMNS, BUFFER> *game_ptr = nullptr;
            Engine::Countdown fps = {0.4/COLUMNS, true}; // 0.375

            template<typename Callback>
            bool operator()(double delta_t, Callback scoreboard_callback) {
                if (m_act == Action::BEGIN) {
                    get_ranges();
                    m_act = Action::CLEARING;
                    return false;
                } else if (m_act == Action::CLEARING) {
                    if (!fps.done(delta_t)) {
                        return false;
                    }
                    if (m_clearing_l < 0 || m_clearing_r >= COLUMNS) {
                        m_act = Action::ENDING;
                        return false;
                    }
                    for (auto it = m_ranges_to_remove.begin(); it != m_ranges_to_remove.end(); ++it) {
                        auto [fst, snd] = *it;
                        for (int i = fst; i < snd; i++) {
                            game_ptr->m_playfield_matrix[i * COLUMNS + m_clearing_l] = Color{};
                            game_ptr->m_playfield_matrix[i * COLUMNS + m_clearing_r] = Color{};
                        }
                    }
                    m_clearing_l -= 1;
                    m_clearing_r += 1;
                    return false;
                } else if (m_act == Action::ENDING) {
                    m_act = Action::BEGIN;
                    pull_down();
                    scoreboard_callback(m_cleared);
                    reset();
                    return true;
                }
                return true;
            }
        private:
            enum class Action {
                BEGIN,
                CLEARING,
                ENDING
            };
            
            std::vector<std::pair<int, int>> m_ranges_to_remove;
            Action m_act = Action::BEGIN;
            int m_clearing_r = COLUMNS / 2;
            int m_clearing_l = COLUMNS % 2 == 0 ? (COLUMNS / 2) - 1 : COLUMNS / 2;
            int m_cleared = 0;
            
            void reset() {
                m_clearing_r = COLUMNS / 2;
                m_clearing_l = COLUMNS % 2 == 0 ? (COLUMNS / 2) - 1 : COLUMNS / 2;
                m_ranges_to_remove.clear();
                m_cleared = 0;
            }

            void get_ranges(void) {
                constexpr int expected_max_ranges = 2;
                m_ranges_to_remove.reserve(expected_max_ranges);
                int curr = 0;
                int prev = 0;
                int rows_to_clean_n = std::count(game_ptr->m_line_block_count.begin(), game_ptr->m_line_block_count.end(), 10);
                // calculates which ranges have to be removed. range is [n,m), n = inclusive, m = exclusive
                // this loop also deletes the rows, which prevents implementing animations !!! for now
                // start from the bottom
                for (int i = 0; i < ROWS + BUFFER && rows_to_clean_n > 0; ++i) {
                    if (game_ptr->m_line_block_count[i] == 10) {
                        rows_to_clean_n--;
                        // std::fill(
                        //     m_playfield_matrix.begin() + (i * COLUMNS),
                        //     m_playfield_matrix.begin() + ((i * COLUMNS + COLUMNS)),
                        //     Color{}
                        // );
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

            void pull_down() {
                // for (int k = m_ranges_to_remove.size() - 1; k >= 0; --k) {
                for (auto it = m_ranges_to_remove.rbegin(); it != m_ranges_to_remove.rend(); ++it) {
                    // const auto [fst, snd] = m_ranges_to_remove[k];
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
        } update_playfield_routine;

        template<typename Callback>
        bool update_playfield_now(double delta_t, Callback scoreboard_callback) {
            std::array<std::pair<int, int>, 10> ranges_to_remove;
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
            // also, use this to calculate the points
            ranges_to_remove[count] = std::make_pair(prev, curr);
            int cleared = 0;
            count++;
            for (int k = count - 1; k >= 0; --k) {
                // std::pair<int, int>& range = ranges_to_remove[k];
                const auto [fst, snd] = ranges_to_remove[k];
                cleared += (snd - fst);
                std::move(
                    m_playfield_matrix.begin() + (snd * COLUMNS),
                    m_playfield_matrix.end(),
                    m_playfield_matrix.begin() + (fst * COLUMNS)
                );
                std::fill(
                    m_playfield_matrix.end() - ((snd - fst) * COLUMNS),
                    m_playfield_matrix.end(),
                    Color{}
                );
                std::move(m_line_block_count.begin() + snd, m_line_block_count.end(), m_line_block_count.begin() + fst);
                std::fill(m_line_block_count.end() - (snd - fst), m_line_block_count.end(), 0);
            }
            scoreboard_callback(cleared);
            return true;
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

        // returns false when a respawn is not needed (or simply when the piece wasn't held)
        bool perform_hold() {
            if (!m_hold_recently_swapped) {
                bool needs_spawn = false;
                if (m_held.get_type() == Tetrimino::Type::NONE) {
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
                kicks = &__Internal::WallkickData::WALLKICKS_I.at(p.get_rotation_state());
            } else {
                kicks = &__Internal::WallkickData::WALLKICKS_ANY.at(p.get_rotation_state());
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

        void draw_piece(SDL_Renderer *renderer, const Tetrimino::Piece& p, bool fill = true, Uint8 alpha = SDL_ALPHA_OPAQUE){
            Color c = p.get_color();
            SDL_BlendMode mode;
            SDL_GetRenderDrawBlendMode(renderer, &mode);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, alpha);
            for (auto [x, y] : p) {
                SDL_FRect r{ .x = x * m_scale + m_board_offset_x, .y = ((ROWS - 1) - y) * m_scale + m_board_offset_y, .w = m_scale, .h = m_scale };
                if (fill) {
                    SDL_RenderFillRect(renderer, &r);
                } else {
                    SDL_RenderRect(renderer, &r);
                }
            }
            SDL_SetRenderDrawBlendMode(renderer, mode);
        };

        void draw_held_piece(SDL_Renderer* renderer, const Tetrimino::Piece& p, float offset_x, float offset_y, float scale){
            Color c = p.get_color();
            SDL_BlendMode mode;
            SDL_GetRenderDrawBlendMode(renderer, &mode);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            const auto type = p.get_type();
            for (auto [x, y] : p.get_blocks()) {
                float extra_off_x = 2;
                float extra_off_y = 3;
                if (type == Tetrimino::Type::I) {
                        extra_off_x = 0.55;
                        // extra_off_y = 3;
                    } else if (type == Tetrimino::Type::O) {
                        extra_off_x = 1.5;

                    }
                SDL_FRect r{ .x = (x + extra_off_x) * scale + offset_x, .y = offset_y - (y - extra_off_y) * scale, .w = scale, .h = scale };
                SDL_RenderFillRect(renderer, &r);
            }
            SDL_SetRenderDrawBlendMode(renderer, mode);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            // verttical
            SDL_RenderLine(renderer,
                offset_x,
                offset_y,
                offset_x,
                offset_y + 5 * scale
            );
            // verttical
            SDL_RenderLine(renderer,
                offset_x + 5 * scale,
                offset_y,
                offset_x + 5 * scale,
                offset_y + 5 * scale
            );
            // horizontal
            SDL_RenderLine(renderer,
                offset_x,
                offset_y,
                offset_x + 5 * scale,
                offset_y
            );
            // horizontal
            SDL_RenderLine(renderer,
                offset_x,
                offset_y + 5 * scale,
                offset_x + 5 * scale,
                offset_y + 5 * scale
            );
        }

        void draw_playfield_blocks(SDL_Renderer *renderer) {
            for (int i = 0; i < ROWS - 2; i++) {
                for (int j = 0; j < COLUMNS; j++) {
                    Color c = m_playfield_matrix[j + i * COLUMNS];
                    if (c.a != 0) {
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
                        SDL_FRect r = {j * m_scale + m_board_offset_x, ((ROWS - 1) - i) * m_scale + m_board_offset_y, m_scale, m_scale};
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
        }

        void draw_playfield_grid(SDL_Renderer *renderer) {
            SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
            for (int i = 1; i < COLUMNS; i++) {
                SDL_RenderLine(renderer,
                    i * m_scale + m_board_offset_x,
                    ROWS * m_scale + m_board_offset_y,
                    i * m_scale + m_board_offset_x,
                    (4 * m_scale + m_board_offset_y) - (2 * m_scale)
                );
            }

            for (int i = 1; i < ROWS - 1; i++) {
                SDL_RenderLine(renderer,
                    m_board_offset_x,
                    (ROWS - i) * m_scale + m_board_offset_y,
                    COLUMNS * m_scale + m_board_offset_x,
                    (ROWS - i) * m_scale + m_board_offset_y
                );
            }

            float thick = m_scale < 10 ? 1 : 4;
            float extra_spacing_top = 1; // [0,1], the smaller, the bigger the extra space
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, SDL_ALPHA_OPAQUE);
            SDL_FRect vertical1 = {
                m_board_offset_x - thick,
                2 * m_scale + m_board_offset_y,
                thick,
                (ROWS - 1) * m_scale - (extra_spacing_top * m_scale)
            };
            SDL_RenderFillRect(renderer, &vertical1);
            SDL_FRect vertical2 = {
                COLUMNS * m_scale + m_board_offset_x,
                vertical1.y,
                thick,
                vertical1.h
            };
            SDL_RenderFillRect(renderer, &vertical2);
            SDL_FRect horizontal1 = {
                m_board_offset_x - thick,
                2 * m_scale - thick + m_board_offset_y,
                COLUMNS * m_scale + thick * 2,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal1);
            SDL_FRect horizontal2 = {
                horizontal1.x,
                ((ROWS - 1 + 2) * m_scale + m_board_offset_y) - (extra_spacing_top * m_scale),
                horizontal1.w,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal2);
        }
    };
}