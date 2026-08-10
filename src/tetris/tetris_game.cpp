#include <memory>
#include <algorithm>

#include "tetris_game.hpp"
#include "tetris_utils.hpp"

namespace Tetris {
    Game::Game(int screen_width, int screen_height, float block_scale) :
        m_scale{block_scale},
        m_board_offset_x{(screen_width - COLUMNS * block_scale) / 2.0f},
        m_board_offset_y{(screen_height - ROWS * block_scale) / 2.0f}
    {
        m_playfield_matrix.fill(Color{});
        m_line_block_count.fill(0);

        // ?
        m_scoreboard.default_speed = m_piece_drop_timer.get_countdown_time();
        m_scoreboard.speed = m_piece_drop_timer.get_countdown_time();

        // Ideally, this wouldn't be a pointer, but I'm tired of templates...
        m_pieces_bag = std::make_unique<Bag::Standard>();

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
    }

    bool Game::update(double delta_t) {
        const auto update_tmp = [this, delta_t]() -> bool {
            if (update_currentpiece(delta_t)) {
                update_ghostpiece();
                return true;
            }
            return false;
        };

        switch (m_update_state) {
            case GameUpdateState::GU_PIECE: {
                // TODO - right now, the game will reset immediately after the game is over. change this to something else later, idk
                if (!update_tmp()) {
                    m_update_state = GameUpdateState::GU_GAMEOVER;
                    // m_game_over = true;
                    restart();
                    return false;
                }
                if (std::find(m_line_block_count.begin(), m_line_block_count.end(), COLUMNS) != m_line_block_count.end()) {
                    if (!update_playfield(delta_t)) { // if not yet fully updated
                        m_update_state = GameUpdateState::GU_PLAYFIELD;
                    }
                }
                return true;
            } break;

            case GameUpdateState::GU_PLAYFIELD: {
                if (update_playfield(delta_t)) {
                    m_update_state = GameUpdateState::GU_PIECE;
                    return update_tmp();
                }
                return true;
            } break;

            case GameUpdateState::GU_GAMEOVER: {
                restart();
                return false;
            } break;

            default:
                std::clog << "Tetris::Game::update(double): reached default case somehow! oops\n";
                // std::abort();
                return false;
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

    void Game::draw(SDL_Renderer *renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        draw_piece(renderer, m_ghost, true, 128);
        draw_piece(renderer, m_current);
        draw_playfield_blocks(renderer);
        draw_playfield_grid(renderer);

        float new_scale = m_scale / 1.5f;
        draw_held_piece(renderer, m_held, m_board_offset_x - 4.40 * m_scale, m_board_offset_y + 2 * m_scale, new_scale);
        m_pieces_bag->draw(renderer, m_board_offset_x + (COLUMNS + 1) * m_scale, m_board_offset_y + 6 * m_scale, new_scale);
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

    bool Game::update_currentpiece(double delta_t) {
        // no need for states here... it's too simple for all that
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
            return place_and_spawn_next_piece();
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
            // to have infinite placement lock down, just remove the lock.moves check
            if ((moved || rotated) && m_piece_lock.moves > 0) {
                m_piece_lock.moves -= 1;
                m_piece_lock.timer.reset();
                // reset_piece_lock_timer();
            } else if (m_piece_lock.timer.done(delta_t)) {
                return place_and_spawn_next_piece();
            }
        } else {
            if (moved_down) {
                reset_piece_lock_timer();
            }
            
            // this timer will auto-reset itself (and by the place_and_spawn_next_piece() function as well)
            if (m_piece_drop_timer.done(delta_t)) {
                if (m_min.y > 0 && !moved_down) {
                    m_current.move(Vec2{ 0, -1 });
                    reset_piece_lock_timer();
                    if (check_collision(m_current)) {
                        m_current.move(Vec2{ 0, 1 });
                        return place_and_spawn_next_piece();
                    }
                } else if (m_min.y <= 0) {
                    return place_and_spawn_next_piece();
                }
            }
        }
        return true;
    }

    void Game::update_ghostpiece(void) {
        m_ghost.move_position(m_current.get_position());
        perform_hard_drop(m_ghost);
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
        return true;
    }

    bool Game::place_current_piece(void) {
        // piece placed above the playfield -> game over condition
        if ((m_current.get_min() + m_current.get_position()).y >= ROWS - 2) {
            // m_game_over = true;
            m_current = {};
            return false;
        }
        for (Vec2 v : m_current) {
            m_playfield_matrix[v.x + v.y * COLUMNS] = m_current.get_color();
            m_line_block_count[v.y] += 1;
        }
        return true;
    }

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
            collides = TetrisUtils::color_to_int32(m_playfield_matrix[v.x + v.y * COLUMNS]) != 0;
        }
        return collides;
    }

    bool Game::perform_hold() {
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

    void Game::draw_piece(SDL_Renderer *renderer, const Tetrimino::Piece& p, bool fill, Uint8 alpha) {
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

    void Game::draw_held_piece(SDL_Renderer* renderer, const Tetrimino::Piece& p, float offset_x, float offset_y, float scale){
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

    void Game::draw_playfield_blocks(SDL_Renderer *renderer) {
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

    void Game::draw_playfield_grid(SDL_Renderer *renderer) {
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

    void Game::ScoreSystem::update(int cleared) {
        int multiplier = 100;
        if (cleared >= 2) multiplier += 200;
        if (cleared >= 3) multiplier += 200;
        if (cleared >= 4) multiplier += 300;
        score += multiplier * level;

        lines_cleared += cleared;
        if (level < max_level && lines_cleared >= goal) {
            level  = (level + 1) % max_level;
            double res = 1;
            for (int exp = level - 1; exp > 0; --exp) {
                res *= (0.8 - ((level - 1) * 0.007));
            }
            speed = res;
            goal = level * lines_to_clear_per_level_multiplier;
            lines_cleared = 0;
        }
    }

    void Game::ScoreSystem::reset(void) {
        score = 0;
        level = 1;
        lines_cleared = 0;
        speed = default_speed;
    }

    void Game::ScoreSystem::set_level(int level) {
        if (level > 1) {
            goal = level * lines_to_clear_per_level_multiplier;
            this->level = level;
            speed = calculate_speed();
        } else {
            goal = 5;

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
        int rows_to_clean_n = std::count(game_ptr->m_line_block_count.begin(), game_ptr->m_line_block_count.end(), COLUMNS);
        for (int i = 0; i < ROWS + BUFFER && rows_to_clean_n > 0; ++i) {
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
        int rows_to_clean_n = std::count(game_ptr->m_line_block_count.begin(), game_ptr->m_line_block_count.end(), COLUMNS);
        // calculates which ranges have to be removed. range is [n,m), n = inclusive, m = exclusive
        // this loop also deletes the rows, which prevents implementing animations !!! for now
        // start from the bottom
        for (int i = 0; i < ROWS + BUFFER && rows_to_clean_n > 0; ++i) {
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