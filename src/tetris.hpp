#pragma once
#include <unordered_map>
#include <array>

#include "tetris_base.hpp"
// #include "tetris_utils.hpp" // rotstr_to_int

namespace Tetris {
    namespace __WallkickData {
        // some of the kick tests are the exact same for some rotations
        // only contains non-zero tests (i.e. no test Vec2 is equal to {0,0})
        extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_ANY;
        // some of the kick tests are the exact same for some rotations
        // only contains non-zero tests (i.e. no test is equal to {0,0})
        extern const std::unordered_map<int, std::array<Vec2, 4>> WALLKICKS_I;
    }

    // matrix grows UPWARDS and to the right. must be flipped when rendered
    // note: not trivially copyable (duh)
    // note2: fully defined in header because template
    template<BagGenerator Bag, int ROWS = 22, int COLUMNS = 10, int BUFFER = 5>
    class Game {
    public:
        bool init(SDL_Texture *playfield_rendertexture) {
            playfield_rtexture = playfield_rendertexture;
            started = true;
            spawn_new_current_piece();
            update_ghostpiece();
            playfield_matrix.fill(Color{0,0,0,0});
            return true;
        }
        // what to do:
        // - update grid when piece is locked
        bool update(double delta_t) {
            if (!started) {
                return false;
            }
            update_currentpiece(delta_t);
            update_ghostpiece();
            return true;
        }
        void move(Vec2 dir) {
            // no upwards movement allowed
            // if (m_nextDir == __invalid_move_vec && dir.y <= 0) {
                m_nextDir = dir;
            // }
        }
        void rotate(Tetrimino::Rotation r) {
            // if (m_nextrot == Tetrimino::Rotation::NONE) {
                m_nextrot = r;
            // }
        }
        // TODO - WORKS, BUT THESE ACTIONS SHOULD NOT UPDATE THE GAME OUTSIDE OF THE UPDATE FUNCTION.
        // MOVE THIS TO UPDATE FUNCTION (LATER)
        void do_hard_drop(void) {
            while (!check_collision(m_current)) {
                m_current.move(Vec2{0,-1});
            }
            // we hit something... move up to unstuck
            m_current.move(Vec2{0,1});
            place_current_piece();
            spawn_new_current_piece();
        }
        // drawing logic is vertically mirrored
        // the playfield grows upwards, but the drawing grows downwards, so we flip it
        void draw(SDL_Renderer *renderer) {
            SDL_SetRenderTarget(renderer, playfield_rtexture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);
            // // debug block
            // {
                // Vec2 min = m_current.get_min() + m_current.get_position(), max = m_current.get_max() + m_current.get_position();
                // SDL_FRect r_min{(min.x + board_offset) * scale, (min.y + board_offset) * scale, scale, scale};
                // SDL_FRect r_max{(max.x + board_offset) * scale, (max.y + board_offset) * scale, scale, scale};
                // std::cout << min << ' ' << max << std::endl;

            //     SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            //     SDL_RenderFillRect(renderer, &r_min);
            //     SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            //     SDL_RenderFillRect(renderer, &r_max);
            // }

            // draws buffer area;
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            for (int i = ROWS - 2; i < ROWS + BUFFER; i++) {
                for (int j = 0; j < COLUMNS; j++) {
                    // Block b = playfield_matrix[j + i * COLUMNS];
                    SDL_FRect r{ .x = (j + board_offset) * scale, .y = (i + board_offset) * scale, .w = scale, .h = scale };
                    // SDL_FRect r{ .x = (b.pos.x + board_offset) * scale, .y = (b.pos.y + board_offset) * scale, .w = scale, .h = scale };
                    // SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, SDL_ALPHA_OPAQUE);
                    SDL_RenderRect(renderer, &r);
                }
            }

            Color c = m_current.get_color();
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            for (Vec2 v : m_current) {
                // Vec2 v = (v.pos + k->get_position());
                SDL_FRect r{ .x = (v.x + board_offset) * scale, .y = (v.y + board_offset) * scale, .w = scale, .h = scale };
                SDL_RenderFillRect(renderer, &r);
            }
            draw_playfield_blocks(renderer);
            // playfield drawn on top of everything
            draw_playfield_grid(renderer);

            SDL_SetRenderTarget(renderer, nullptr);
            // SDL_RenderTexture(renderer, playfield_rtexture, nullptr, nullptr);
            m_pieces_bag.draw(renderer, nullptr);
        }
    private:
        // TODO - organize this
        inline static constexpr Vec2 __invalid_move_vec = { 0xFFFF, 0xFFFF };
        std::array<Color, (ROWS + BUFFER) * COLUMNS> playfield_matrix = {}; // +5 rows of extra buffer
        std::array<int, ROWS + BUFFER> line_block_count = {};

        Bag m_pieces_bag{};
        Tetrimino::Piece m_current{};
        Tetrimino::Piece m_ghost{};
        Tetrimino::Rotation m_nextrot = Tetrimino::Rotation::NONE;
        Vec2 m_nextDir = __invalid_move_vec;

        bool started = true;
        bool hard_drop = false;
        float board_offset = 1;
        float scale = 30;
        double piece_drop_timer_acc = 0;

        SDL_Texture *playfield_rtexture = nullptr;
        
        bool update_currentpiece(double delta_t) {
            bool rotated = false;
            piece_drop_timer_acc += delta_t;
            if (m_nextrot != Tetrimino::Rotation::NONE) {
                m_current.rotate(m_nextrot);
                if (!perform_wallkick(m_current)) {
                    m_current.rotate(Tetrimino::get_opposite_rotation(m_nextrot));
                } else {
                    rotated = true;
                }
                m_nextrot = Tetrimino::Rotation::NONE;
            }
            bool moved = false;
            bool moved_downwards = false;
            Vec2 m_min = {m_current.get_min() + m_current.get_position()};
            if (m_nextDir != __invalid_move_vec) {
                if (m_nextDir.x != 0) {
                    m_nextDir.y = 0; // prioritize horizontal moves
                } else if (m_nextDir.y < 0) {
                    piece_drop_timer_acc = 0;
                    moved_downwards = true;
                }
                m_current.move(m_nextDir);
                // FIXME - collision against walls should NOT lock the piece down (apparenty fixed? by changing how input is handled from the outside)
                // separate check of wall collisions and block collisions
                // (or maybe separate the frames in which downwards and sideways moves happen)
                if (!check_within_bounds(m_current)) {
                    m_current.move(m_nextDir * -1); // we hit a wall. undo move
                    if (moved_downwards) {
                        place_current_piece();
                        spawn_new_current_piece();
                    }
                } else if (check_collision(m_current)) {
                    m_current.move(m_nextDir * -1); // we hit a wall. undo move
                    if (moved_downwards) {
                        place_current_piece();
                        spawn_new_current_piece();
                    }
                }
                m_min = m_current.get_min() + m_current.get_position();
                moved = true;
                if (m_min.y < 0) {
                    place_current_piece();
                    spawn_new_current_piece();
                }
                m_nextDir = __invalid_move_vec;
            }

            // LOCK PIECE DOWN IF DOWNWARDS MOVEMENT IS IMPOSS8IBLE AND 
            // STOP LOCKDOWN TIMER IF SIDEWAYS MOVEMENT OR ROTATION IS DETECTED
            if (piece_drop_timer_acc >= 1) {
                piece_drop_timer_acc = 0;
                if (m_min.y > 0 && !moved) {
                    m_current.move(Vec2{ 0, -1 });
                    if (check_collision(m_current)) {
                        m_current.move(Vec2{ 0, 1 });
                        place_current_piece();
                        spawn_new_current_piece();
                    }
                } else if (m_min.y <= 0) {
                    place_current_piece();
                    spawn_new_current_piece();
                }
            }
            return true;
        }

        // TODO - implement these
        void update_ghostpiece(void) {
        }

        // just pulls the next piece out of the bag
        void spawn_new_current_piece(void) {
            m_current = m_pieces_bag();
        }

        void place_current_piece(void) {
            for (Vec2 v : m_current) {
                playfield_matrix[v.x + v.y * COLUMNS] = m_current.get_color();
            }
            m_current = {};
        }

        void update_matrix(double delta_t) {
            
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
                collides = playfield_matrix[v.x + v.y * COLUMNS].a != 0;
            }
            return collides;
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
            for (int i = 0; i < ROWS; i++) {
                for (int j = 0; j < COLUMNS; j++) {
                    Color c = playfield_matrix[j + i * COLUMNS];
                    if (c.a != 0) {
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
                        SDL_FRect r = {(j + board_offset) * scale, (i + board_offset) * scale, scale, scale};
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
        }

        void draw_playfield_grid(SDL_Renderer *renderer) {
            SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
            for (int i = 1; i < COLUMNS; i++) {
                SDL_RenderLine(renderer,
                    (i + board_offset) * scale,
                    board_offset * scale,
                    (i + board_offset) * scale,
                    ((ROWS + board_offset) * scale) - (2 * scale)
                );
            }

            for (int i = 1; i < ROWS - 1; i++) {
                SDL_RenderLine(renderer,
                    board_offset * scale,
                    (i + board_offset) * scale,
                    (COLUMNS + board_offset) * scale,
                    (i + board_offset) * scale
                );
            }

            float thick = scale < 10 ? 1 : 3 * 2;
            float extra_spacing_top = 1; // [0,1], the smaller, the bigger the extra space
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, SDL_ALPHA_OPAQUE);
            SDL_FRect vertical1 = {
                board_offset * scale - thick,
                board_offset * scale,
                thick,
                (board_offset + ROWS - 2) * scale - (extra_spacing_top * scale)
            };
            SDL_RenderFillRect(renderer, &vertical1);
            SDL_FRect vertical2 = {
                (board_offset + COLUMNS) * scale,
                vertical1.y,
                thick,
                vertical1.h
            };
            SDL_RenderFillRect(renderer, &vertical2);
            SDL_FRect horizontal1 = {
                board_offset * scale - thick,
                board_offset * scale - thick,
                (board_offset + COLUMNS - 1) * scale + thick * 2,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal1);
            SDL_FRect horizontal2 = {
                horizontal1.x,
                (board_offset + ROWS - 1) * scale - (extra_spacing_top * scale),
                horizontal1.w,
                thick
            };
            SDL_RenderFillRect(renderer, &horizontal2);
        }
    };
}