#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <random>
#include "tetris_bags.hpp"
// #include "tetris_utils.hpp"
#include "../engine/tengine.hpp"
#include "../engine/utils.hpp"
#include "tetris_pieces.hpp"

namespace Tetris{
    namespace Bag {
        // RANDOM!!!
        Random::Random(SDL_Renderer *renderer, SDL_Texture* mino, float offset_x, float offset_y, float block_scale) :
            m_range_min{Tetrimino::min_type_as_int()},
            m_range_max{Tetrimino::max_type_as_int()},    
            m_renderer{renderer},
            m_mino{mino},
            m_offset_x{offset_x},
            m_offset_y{offset_y},
            m_block_scale{block_scale}
        {}

        Tetrimino::Piece Random::operator()() {
            int piece_idx = m_next;
            m_next = TEngine::random_int(m_range_min, m_range_max);
            return Tetrimino::get_piece(static_cast<Tetrimino::Type>(piece_idx));
        }

        void Random::draw() {
            const auto& p = Tetrimino::get_piece(static_cast<Tetrimino::Type>(m_next));
            TEngine::Color c = p.get_color();
            Tetrimino::Type type = p.get_type();
            SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            for (auto [x, y] : p.get_blocks()) {
                float extra_off_x = 2;
                float extra_off_y = 3;
                if (type == Tetrimino::Type::I) {
                    extra_off_x = 0.5;
                    extra_off_y = 1.5;
                } else if (type == Tetrimino::Type::O) {
                    extra_off_x = 1.5;
                }
                SDL_FRect r {
                    .x = (extra_off_x + x) * m_block_scale + m_offset_x,
                    .y = m_offset_y - (y - extra_off_y) * m_block_scale,
                    .w = m_block_scale, 
                    .h = m_block_scale
                };
                SDL_RenderFillRect(m_renderer, &r);
            }

            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            // verttical
            SDL_RenderLine(m_renderer,
                m_offset_x,
                m_offset_y,
                m_offset_x,
                m_offset_y + m_block_scale * 5
            );
            // verttical
            SDL_RenderLine(m_renderer,
                m_offset_x + 5 * m_block_scale,
                m_offset_y,
                m_offset_x + 5 * m_block_scale,
                m_offset_y + m_block_scale * 5
            );
            // horizontal
            SDL_RenderLine(m_renderer,
                m_offset_x,
                m_offset_y,
                m_offset_x + 5 * m_block_scale,
                m_offset_y
            );
            // horizontal
            SDL_RenderLine(m_renderer,
                m_offset_x,
                m_offset_y + m_block_scale * 5,
                m_offset_x + 5 * m_block_scale,
                m_offset_y + m_block_scale * 5
            );
        }

        void Random::reset() {
            m_next = TEngine::random_int(m_range_min, m_range_max);
        }

        // 7 BAG!!!
        Standard::Standard(SDL_Renderer *renderer, SDL_Texture* mino, float offset_x, float offset_y, float block_scale) :
            m_random_engine{std::random_device{}()},
            m_renderer{renderer},
            m_mino{mino},
            m_offset_x{offset_x},
            m_offset_y{offset_y},
            m_block_scale{block_scale},
            m_pieces{},
            m_bag_pool{},
            m_verts{},
            m_indices{}
        {
            m_pieces.reserve(static_cast<int>(Tetrimino::get_amount_of_pieces()));
            for (const auto& [type, _] : Tetrimino::get_all_pieces()) {
                if (type != Tetrimino::Type::None && type != Tetrimino::Type::Custom) {
                    m_pieces.push_back(type);
                }
            }
            // m_dist = std::uniform_int_distribution<int>{0, static_cast<int>(m_pieces.size() - 2)};
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            m_bag_pool = m_pieces;
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            regen_geometry();
        }

        // effectively the 7Bag system... kinda? might need some extra work
        Tetrimino::Piece Standard::operator()() {
            Tetrimino::Type piece_type = m_bag_pool.back();
            m_bag_pool.pop_back();
            if (m_bag_pool.empty()) {
                m_bag_pool = m_pieces;
                std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            }
            // m_prev_type = piece_type;
            regen_geometry();
            return Tetrimino::get_piece(piece_type);
        }

        void Standard::draw() {
            int lim = static_cast<int>(Tetrimino::get_amount_of_pieces()) - 1; // -1 avoids the risk of showing too many repeated pieces
            SDL_RenderGeometry(
                m_renderer,
                m_mino,
                m_verts.data(),
                static_cast<int>(m_verts.size()),
                m_indices.data(),
                static_cast<int>(m_indices.size())
            );
            
            float thick = 4;
            if (m_block_scale < 10) {
                thick = 1;
            }
            std::array<SDL_FRect, 4> rects{};
            rects[0] = {
                m_offset_x - thick,
                m_offset_y,
                thick,
                lim * m_block_scale * 4
            };
            rects[1] = {
                m_offset_x + 5 * m_block_scale,
                rects[0].y,
                thick,
                rects[0].h
            };
            rects[2] = {
                m_offset_x - thick,
                m_offset_y - thick,
                5 * m_block_scale + thick * 2,
                thick
            };
            rects[3] = {
                rects[2].x,
                m_offset_y + lim * m_block_scale * 4,
                rects[2].w,
                thick
            };
            TEngine::Color c = TEngine::TUtils::color_from_hex("#808080");
            SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRects(m_renderer, rects.data(), static_cast<int>(rects.size()));
        }

        void Standard::reset() {
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            m_bag_pool = m_pieces;
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
        }

        void Standard::regen_geometry() {
            m_verts.clear();
            m_indices.clear();

            int i = 0;
            int lim = static_cast<int>(Tetrimino::get_amount_of_pieces()) - 1; // -1 avoids the risk of showing too many repeated pieces
            // crosses bag boundaries, displaying all the pieces that will come up next (all 7 of them),
            // even if they're not necessarily in m_bag_pool
            int v_idx = 0;
            const auto gen_full_bag = [this, lim, &i, &v_idx](const std::vector<Tetrimino::Type>& vec){
                for (auto it = vec.rbegin(); it != vec.rend() && i < lim; ++it) {
                    const auto& p = Tetrimino::get_piece(*it);
                    TEngine::Color c = p.get_color();
                    SDL_FColor color {
                        c.r / 255.f,
                        c.g / 255.f,
                        c.b / 255.f,
                        c.a / 255.f
                    };
                    const auto type = p.get_type();
                    for (auto [x, y] : p.get_blocks()) {
                        float extra_off_x = 2;
                        float extra_off_y = 2;
                        if (type == Tetrimino::Type::I) {
                            extra_off_x = 0.5;
                            extra_off_y = 1.5;
                        } else if (type == Tetrimino::Type::O) {
                            extra_off_x = 1.5;
                        }
                        SDL_FRect r {
                            .x = (x + extra_off_x) * m_block_scale + m_offset_x,
                            .y = m_offset_y - (y - extra_off_y) * m_block_scale + (m_block_scale * i) * 4,
                            .w = m_block_scale,
                            .h = m_block_scale
                        };

                        SDL_Vertex v0 = {
                            {r.x, r.y},
                            color,
                            {0, 0}
                        };

                        SDL_Vertex v1 = {
                            {r.x + r.w, r.y},
                            color,
                            {1, 0}
                        };

                        SDL_Vertex v2 = {
                            {r.x + r.w, r.y + r.h},
                            color,
                            {1, 1}
                        };

                        SDL_Vertex v3 = {
                            {r.x, r.y + r.h},
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
                    i++;
                }
            };

            gen_full_bag(m_bag_pool);
            gen_full_bag(m_pieces);
        }
    };
}