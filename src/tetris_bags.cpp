#include "tetris_bags.hpp"

namespace Tetris{
    namespace Bag {
        // RANDOM!!!
        Random::Random() :
            m_bag{},
            rd{},
            m_random_engine{rd()},
            m_dist{static_cast<int>(Tetrimino::Type::NONE) + 1, static_cast<int>(Tetrimino::Type::CUSTOM) - 1}
        {
            m_next = m_dist(m_random_engine);
        }

        Tetrimino::Piece Random::operator()() {
            int piece_idx = m_next;
            m_next = m_dist(m_random_engine);
            return Tetrimino::get_piece(static_cast<Tetrimino::Type>(piece_idx));
        }

        void Random::draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) {
            const auto& p = Tetrimino::get_piece(static_cast<Tetrimino::Type>(m_next));
            Color c = p.get_color();
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            for (auto [x, y] : p.get_blocks()) {
                SDL_FRect r { .x = x * block_scale + offset_x, .y = offset_y - y * block_scale, .w = block_scale, .h = block_scale };
                SDL_RenderFillRect(renderer, &r);
            }
        }

        void Random::reset() {
            m_next = m_dist(m_random_engine);
        }

        // 7 BAG!!!
        Standard::Standard() :
            m_pieces{},
            m_bag_pool{},
            rd{},
            m_random_engine{rd()}
        {
            m_pieces.reserve(static_cast<int>(Tetrimino::get_amount_of_pieces()));
            for (const auto& [type, _] : Tetrimino::get_all_pieces()) {
                if (type != Tetrimino::Type::NONE && type != Tetrimino::Type::CUSTOM) {
                    m_pieces.push_back(type);
                }
            }
            // m_dist = std::uniform_int_distribution<int>{0, static_cast<int>(m_pieces.size() - 2)};
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            m_bag_pool = m_pieces;
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
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
            return Tetrimino::get_piece(piece_type);
        }

        // just assume the screen coordinated for now.... ignore the texture
        void Standard::draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) {
            int i = 0;
            int lim = static_cast<int>(Tetrimino::get_amount_of_pieces()) - 1; // -1 avoids the risk of showing too many repeated pieces
            // crosses bag boundaries, displaying all the pieces that will come up next (all 7 of them),
            // even if they're not necessarily in m_bag_pool
            const auto draw_full_bag = [renderer, lim, &i, offset_x, offset_y, block_scale](const std::vector<Tetrimino::Type>& vec){
                for (auto it = vec.rbegin(); it != vec.rend() && i < lim; ++it) {
                    const auto& p = Tetrimino::get_piece(*it);
                    Color c = p.get_color();
                    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
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
                        SDL_FRect r { .x = (x + extra_off_x) * block_scale + offset_x, .y = offset_y - (y - extra_off_y) * block_scale + (block_scale * i) * 4, .w = block_scale, .h = block_scale };
                        // SDL_FRect r { .x = x * block_scale + offset_x, .y = offset_y - y * block_scale + (block_scale * i) * 4, .w = block_scale, .h = block_scale };
                        SDL_RenderFillRect(renderer, &r);
                    }
                    i++;
                }
            };
            draw_full_bag(m_bag_pool);
            draw_full_bag(m_pieces);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            // verttical
            SDL_RenderLine(renderer,
                offset_x,
                offset_y,
                offset_x,
                offset_y + lim * block_scale * 4
            );
            // verttical
            SDL_RenderLine(renderer,
                offset_x + 5 * block_scale,
                offset_y,
                offset_x + 5 * block_scale,
                offset_y + lim * block_scale * 4
            );
            // horizontal
            SDL_RenderLine(renderer,
                offset_x,
                offset_y,
                offset_x + 5 * block_scale,
                offset_y
            );
            // horizontal
            SDL_RenderLine(renderer,
                offset_x,
                offset_y + lim * block_scale * 4,
                offset_x + 5 * block_scale,
                offset_y + lim * block_scale * 4
            );
        }

        void Standard::reset() {
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_random_engine);
            m_bag_pool = m_pieces;
        }
    };
}