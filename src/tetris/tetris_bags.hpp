#pragma once

#include <random>
#include <SDL3/SDL.h>

#include "tetris_pieces.hpp"

namespace Tetris {
    namespace Bag {
        // available so we can remove the template from tetris.hpp (dear god)
        class Queue {
        public:
            virtual ~Queue() = default;

            virtual Tetrimino::Piece operator()() = 0;
            virtual void draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) = 0;
            virtual void reset() = 0;
        };
        // mostly for debugging. always returns the same piece
        template<Tetrimino::Type T>
        struct OnePiece : public Queue {
            Tetrimino::Piece operator()() {return Tetrimino::get_piece(T);}
            // these arguments may eventually be used, but currently, they're just here
            // so that the struct is compatible with the TetriminoQueue concept
            void draw(SDL_Renderer* renderer, float, float, float) {}
            void reset() {}
        };

        // unfairly random... not very useful
        class Random : public Queue {
        public:
            Random();
            Tetrimino::Piece operator()() override;
            void draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) override;
            void reset() override;

        private:
            std::random_device rd;
            std::mt19937 m_random_engine;
            std::uniform_int_distribution<int> m_dist;
            int m_next;
            // std::uniform_int_distribution<int> m_random_gen;
        };

        // 7 Bag system. Fairly random and the default option to be used
        class Standard : public Queue {
        public:
            Standard();
            Tetrimino::Piece operator()() override;
            void draw(SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) override;
            void reset() override;

        private:
            std::vector<Tetrimino::Type> m_pieces;
            std::vector<Tetrimino::Type> m_bag_pool;
            std::random_device rd;
            std::mt19937 m_random_engine;
        };

        // Note about the draw function:
        //     offset_x and offset_y (first and second float arguments respectively) have to be in pixels.
        //     block_scale (the last float argument) has to be the size units of each individual block (not pixels) (i.e.: 1, 2 or 3.5).
        template<typename T>
        concept TetriminoQueue = requires(T bag, SDL_Renderer *renderer, float offset_x, float offset_y, float block_scale) {
            { bag() } -> std::same_as<Tetris::Tetrimino::Piece>;
            { bag.draw(renderer, offset_x, offset_y, block_scale) } -> std::same_as<void>;
            { bag.reset() } -> std::same_as<void>;
        } || std::is_base_of_v<Queue, T>;
    };
};