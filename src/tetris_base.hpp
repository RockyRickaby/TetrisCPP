#pragma once

#include <unordered_map>
#include <vector>
#include <iterator>
#include <iostream>
#include <functional>
#include <algorithm>
#include <array>
#include <SDL3/SDL.h>

#if __cplusplus >= 202002L
namespace Tetris { namespace Tetrimino { class Piece; } };
template<typename T>
concept BagGenerator = requires(T bag, SDL_Renderer *renderer, SDL_Texture *texture) {
    { bag() } -> std::same_as<Tetris::Tetrimino::Piece>;
    { bag.draw(renderer, texture) } -> std::same_as<void>;
};
#else
#define BagGenerator typename
#endif

namespace Tetris {
    // already available in SDL as SDL_Color
    struct Color {
        Uint8 r = 0;
        Uint8 g = 0;
        Uint8 b = 0;
        Uint8 a = 0;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };
 
    struct Vec2 {
        float x = 0;
        float y = 0;

        Vec2 operator+(const Vec2 &other) const;
        Vec2 operator-(const Vec2 &other) const;
        Vec2 operator*(float scalar) const;
        Vec2& operator+=(const Vec2 &other);
        Vec2& operator-=(const Vec2 &other);
        Vec2& operator*=(float scalar);
        bool operator==(const Vec2 &other) const;
        bool operator!=(const Vec2 &other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

    // struct Block {
    //     Vec2 pos = {};
    //     Color color = {};

    //     Block operator+(const Vec2 &other) const;
    //     Block operator-(const Vec2 &other) const;
    //     Block& operator+=(const Vec2 &other);
    //     Block& operator-=(const Vec2 &other);
    // };

    namespace Tetrimino {
        enum class Rotation {
            NONE,
            CLOCKWISE,
            COUNTERCLOCKWISE,
        };

        enum class Type {
            NONE, I, J, L, O, S, Z, T, CUSTOM
        };

        class Piece {
        public:
            Piece();
            Piece(Type type, Color color, Vec2 pos, Vec2 center, std::vector<Vec2>&& body = {});
            Type get_type(void) const;
            Color get_color(void) const;
            Vec2 get_position(void) const;
            Vec2 get_min(void) const;
            Vec2 get_max(void) const;
            int get_rotation_state(void) const;
            bool move(Vec2 dir);
            bool move_position(Vec2 pos);
            bool rotate(Rotation r);
            // merely reads the blocks that make up the piece, with positions relative to the piece's
            const std::vector<Vec2>& get_blocks(void) const;
            // grabs a copy of the blocks that make up the piece, with absolute positions for each, storing them on vec
            // vec has to be an empty vector
            size_t get_blocks(std::vector<Vec2> &vec) const;
        private:
            Type type;
            Tetris::Color color;
            Vec2 m_pos;
            Vec2 m_center;
            Vec2 m_min;
            Vec2 m_max;
            std::vector<Vec2> m_body;
            // 0 = 0 -> spawn state
            // 1 = R
            // 2 = 2
            // 3 = L
            int prev_state;
            int curr_state;

            void update_bounds(Vec2 r);
        public:
            class BlockIterator;
            BlockIterator begin() const;
            BlockIterator end() const;

            // not really necessary. just makes it a tiny little bit less annoying to render pieces on the screen
            class BlockIterator {
            private:
                std::vector<Vec2>::const_iterator m_block_it;
                Vec2 m_it_pos;
            public:
                BlockIterator(std::vector<Vec2>::const_iterator it, Vec2 it_pos);
                BlockIterator& operator++();
                BlockIterator operator++(int);
                bool operator==(BlockIterator other);
                bool operator!=(BlockIterator other);
                Vec2 operator*();

                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = std::vector<Vec2>::const_iterator;
                using pointer = const std::vector<Vec2>::const_iterator*;
                using reference = const std::vector<Vec2>::const_iterator&;
                using iterator_category = std::input_iterator_tag;
            };
        };
        extern const std::unordered_map<Type, Piece> DEFAULT_PIECES;
        Piece get_piece(Type type);
        Rotation get_opposite_rotation(Rotation r);
    };

    namespace Bag {
        // mostly for debugging. always returns the same piece
        template<Tetrimino::Type T>
        struct OnePiece {
            Tetrimino::Piece operator()() {return Tetrimino::get_piece(T);}
            void draw(SDL_Renderer *renderer, SDL_Texture *texture) { /*nodraw*/ }
        };

        struct Standard {
            Tetrimino::Piece operator()();
            void draw(SDL_Renderer *renderer, SDL_Texture *texture);
        };

        struct Random {
            Tetrimino::Piece operator()();
            void draw(SDL_Renderer *renderer, SDL_Texture *texture);
        };
    };
}