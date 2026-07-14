#pragma once

#include <unordered_map>
#include <vector>
#include <iterator>
#include <iostream>

namespace Tetris {
    struct Color {
        int r, g, b, a;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };
 
    struct Vec2 {
        float x, y;

        Vec2 operator+(const Vec2 &other) const;
        Vec2 operator-(const Vec2 &other) const;
        Vec2& operator+=(const Vec2 &other);
        Vec2& operator-=(const Vec2 &other);
        bool operator==(const Vec2 &other) const;
        bool operator!=(const Vec2 &other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

    struct Block {
        Vec2 pos;
        Color color;

        Block operator+(const Vec2 &other) const;
        Block operator-(const Vec2 &other) const;
        Block& operator+=(const Vec2 &other);
        Block& operator-=(const Vec2 &other);
    };

    namespace Tetrimino {
        enum class Type;
        class Piece;
        extern const std::unordered_map<Type, Piece> DEFAULT_PIECES;

        enum class Rotation {
            CLOCKWISE,
            COUNTERCLOCKWISE,
        };

        enum class Type {
            NONE, I, J, L, O, S, Z, T, CUSTOM
        };

        class Piece {
        public:
            class BlockIterator;
            const Type type;
            const Tetris::Color color;

            Piece(Type type, Color color, Vec2 pos, Vec2 center, std::vector<Block>&& body = {});
            bool move(Vec2 dir);
            bool move_position(Vec2 pos);
            bool rotate(Rotation r);
            Vec2 get_position(void) const;
            // merely reads the blocks that make up the piece, with positions relative to the piece's
            const std::vector<Block>& get_blocks(void) const;
            // grabs a copy of the blocks that make up the piece, with absolute positions for each, storing them on vec
            // vec has to be an empty vector
            size_t get_blocks(std::vector<Block> &vec) const;
            // Piece& operator=(Piece other);
        private:
            Vec2 m_pos;
            const Vec2 m_center;
            std::vector<Block> m_body;
            
        public:
            BlockIterator begin() const;
            BlockIterator end() const;

            // not really necessary. just makes it a tiny little bit less annoying to render pieces on the screen
            class BlockIterator {
            private:
                std::vector<Block>::const_iterator m_block_it;
                Vec2 m_it_pos;
            public:
                BlockIterator(std::vector<Block>::const_iterator it, Vec2 it_pos);
                BlockIterator& operator++();
                BlockIterator operator++(int);
                bool operator==(BlockIterator other);
                bool operator!=(BlockIterator other);
                Block operator*();

                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = std::vector<Block>::const_iterator;
                using pointer = const std::vector<Block>::const_iterator*;
                using reference = const std::vector<Block>::const_iterator&;
                using iterator_category = std::input_iterator_tag;
            };
        };
    };
}