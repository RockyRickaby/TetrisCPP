#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <SDL3/SDL_render.h>

// Implemented in base.cpp and pieces.cpp
namespace Tetris {
    // already available in SDL as SDL_Color
    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        bool operator==(const Color other) const;
        bool operator!=(const Color other) const;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };
 
    struct Vec2 {
        float x = 0;
        float y = 0;

        Vec2 operator+(const Vec2 other) const;
        Vec2 operator-(const Vec2 other) const;
        Vec2 operator*(float scalar) const;
        Vec2& operator+=(const Vec2 other);
        Vec2& operator-=(const Vec2 other);
        Vec2& operator*=(float scalar);
        bool operator==(const Vec2 other) const;
        bool operator!=(const Vec2 other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

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

            Type get_type(void) const { return m_type; }
            Color get_color(void) const { return m_color; }
            Vec2 get_position(void) const { return m_pos; }
            Vec2 get_min(void) const { return m_min; }
            Vec2 get_max(void) const { return m_max; }
            int get_rotation_state(void) const { return m_prev_state * 10 + m_curr_state; }
            bool move(Vec2 dir);
            bool move_position(Vec2 pos);
            bool rotate(Rotation r);
            // grabs a copy of the blocks that make up the piece, with absolute positions for each, storing them on vec
            // vec has to be an empty vector
            // you may also just use the class' iterator (i.e. use the class instance in a for loop)
            size_t get_blocks(std::vector<Vec2> &vec) const;
            // merely reads the blocks that make up the piece, with positions relative to the piece's
            const std::vector<Vec2>& get_blocks(void) const { return m_body; }
        private:
            Type m_type;
            Tetris::Color m_color;
            Vec2 m_pos;
            Vec2 m_center;
            Vec2 m_min;
            Vec2 m_max;
            std::vector<Vec2> m_body;
            // 0 = 0 -> spawn state
            // 1 = R
            // 2 = 2
            // 3 = L
            int m_prev_state;
            int m_curr_state;

            void update_bounds(Vec2 r);
        public:
            class BlockIterator;
            BlockIterator begin() const { return Piece::BlockIterator{m_body.begin(), m_pos}; }
            BlockIterator end() const { return Piece::BlockIterator{m_body.end(), m_pos}; }

            // not really necessary. just makes it a tiny little bit less annoying to render pieces on the screen
            class BlockIterator {
            private:
                std::vector<Vec2>::const_iterator m_block_it;
                Vec2 m_it_pos;
            public:
                BlockIterator(std::vector<Vec2>::const_iterator it, Vec2 it_pos) : m_block_it{it}, m_it_pos{it_pos} {}
                BlockIterator& operator++() { ++m_block_it; return *this; }
                BlockIterator operator++(int) { BlockIterator b = *this; ++(*this); return b; }
                bool operator==(BlockIterator other) { return m_block_it == other.m_block_it; }
                bool operator!=(BlockIterator other) { return m_block_it != other.m_block_it; }
                Vec2 operator*() { Vec2 r = *m_block_it; r += m_it_pos; return r; }

                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = std::vector<Vec2>::const_iterator;
                using pointer = const std::vector<Vec2>::const_iterator*;
                using reference = const std::vector<Vec2>::const_iterator&;
                using iterator_category = std::input_iterator_tag;
            };
        };
        extern const std::unordered_map<Type, Piece> DEFAULT_PIECES;

        inline int get_amount_of_pieces(void) { return DEFAULT_PIECES.size() - 2; /* two of the pieces are invalid */ }
        // absolutely prevent a copy
        inline const std::unordered_map<Type, Piece>& get_all_pieces() { return DEFAULT_PIECES; } 
        // prevent copy when a reference is all that's needed
        inline const Piece& get_piece(Type type) {
            if (type == Type::NONE) {
                // return Piece{};
            } else if (type == Type::CUSTOM) {
                // return Piece{};
            }
            return DEFAULT_PIECES.at(type);
        }

        inline Rotation get_opposite_rotation(Rotation r) {
            if (r == Rotation::CLOCKWISE) {
                return Rotation::COUNTERCLOCKWISE;
            } else if (r == Rotation::COUNTERCLOCKWISE) {
                return Rotation::CLOCKWISE;
            } else {
                return Rotation::NONE;
            }
        }
    };
}