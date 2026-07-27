#pragma once

#include <algorithm>
#include <deque>
#include <random>
#include <SDL3/SDL_render.h>

#include "tetris_base.hpp"

#if __cplusplus >= 202002L
namespace Tetris { namespace Tetrimino { class Piece; } };
template<typename T>
concept TetriminoQueue = requires(T bag, SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) {
    { bag() } -> std::same_as<Tetris::Tetrimino::Piece>;
    { bag.draw(renderer, offset_x, offset_y, block_scale) } -> std::same_as<void>;
    { bag.reset() } -> std::same_as<void>;
};
#else
#define TetriminoQueue typename
#endif

namespace Tetris {
    namespace Bag {
        // mostly for debugging. always returns the same piece
        template<Tetrimino::Type T>
        struct OnePiece {
            Tetrimino::Piece operator()() {return Tetrimino::get_piece(T);}
            // these arguments may eventually be used, but currently, they're just here
            // so that the struct is compatible with the TetriminoQueue concept
            void draw(SDL_Renderer* renderer, float, float, float) {
                const auto& p = Tetrimino::get_piece(T);
                Color c = p.get_color();
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
                for (const auto& [x, y] : p.get_blocks()) {
                    SDL_FRect r { .x = (x + 20) * 20, .y = (y + 20) * 20, .w = 20, .h = 20 };
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        };

        // unfairly random... not very useful
        class Random {
        public:
            Random();
            Tetrimino::Piece operator()();
            void draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale);
            void reset();

        private:
            std::deque<Tetrimino::Type> m_bag;
            std::random_device rd;
            std::mt19937 m_random_engine;
            std::uniform_int_distribution<int> m_dist;
            int m_next;
            // std::uniform_int_distribution<int> m_random_gen;
        };

        // 7 bag system. Fairly random and the default option to be used
        class Standard {
        public:
            Standard();
            Tetrimino::Piece operator()();
            void draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale);
            void reset();

        private:
            std::vector<Tetrimino::Type> m_pieces;
            std::vector<Tetrimino::Type> m_bag_pool;
            std::random_device rd;
            std::mt19937 m_random_engine;
        };
    };
};