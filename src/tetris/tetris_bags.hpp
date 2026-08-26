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
            virtual void draw() = 0;
            virtual void reset() = 0;
        };
        // mostly for debugging. always returns the same piece
        template<Tetrimino::Type T>
        struct OnePiece : public Queue {
            OnePiece(SDL_Renderer*, SDL_Texture*, float, float, float) {}
            Tetrimino::Piece operator()() override {return Tetrimino::get_piece(T);}
            // these arguments may eventually be used, but currently, they're just here
            // so that the struct is compatible with the TetriminoQueue concept
            void draw() override {}
            void reset() override {}
        };

        // unfairly random... not very useful
        class Random : public Queue {
        public:
            Random(SDL_Renderer *renderer, SDL_Texture* mino, float offset_x, float offset_y, float block_scale);
            Tetrimino::Piece operator()() override;
            void draw() override;
            void reset() override;

        private:
            std::random_device rd;
            std::mt19937 m_random_engine;
            std::uniform_int_distribution<int> m_dist;
            int m_next;

            SDL_Renderer* m_renderer;
            SDL_Texture* m_mino;
            float m_offset_x;
            float m_offset_y;
            float m_block_scale;
            // std::uniform_int_distribution<int> m_random_gen;
        };

        // 7 Bag system. Fairly random and the default option to be used
        class Standard : public Queue {
        public:
            Standard(SDL_Renderer *renderer, SDL_Texture* mino, float offset_x, float offset_y, float block_scale);
            Tetrimino::Piece operator()() override;
            void draw() override;
            void reset() override;

        private:
            void regen_geometry();

            SDL_Renderer* m_renderer;
            SDL_Texture* m_mino;
            float m_offset_x;
            float m_offset_y;
            float m_block_scale;

            std::vector<Tetrimino::Type> m_pieces;
            std::vector<Tetrimino::Type> m_bag_pool;
            std::random_device rd;
            std::mt19937 m_random_engine;

            std::vector<SDL_Vertex> m_verts;
            std::vector<int> m_indices;
        };

        template<typename T>
        concept TetriminoQueue = requires(T bag, SDL_Renderer *renderer, SDL_Texture* mino, float offset_x, float offset_y, float block_scale) {
            { T(renderer, mino, offset_x, offset_y, block_scale) } -> std::same_as<T>; // constructor
            // { bag() } -> std::same_as<Tetris::Tetrimino::Piece>; // () operator
            // { bag.draw() } -> std::same_as<void>;
            // { bag.reset() } -> std::same_as<void>;
        } && std::is_base_of_v<Queue, T>;
    };
};