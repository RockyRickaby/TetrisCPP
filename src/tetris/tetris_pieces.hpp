#pragma once

#include <vector>
#include <unordered_map>

#include "../engine/tengine.hpp"

namespace Tetris{
    namespace Tetrimino {
        enum class Rotation {
            None,
            Clockwise,
            Counterclockwise,
        };
        
        enum class Type {
            None, I, J, L, O, S, Z, T, Custom
        };
        // non-virtual destructor
        class Piece {
        public:
            Piece();
            Piece(Type type, TEngine::Color color, TEngine::Vec2 pos, TEngine::Vec2 center, std::vector<TEngine::Vec2>&& body = {});

            Type get_type(void) const { return m_type; }
            TEngine::Color get_color(void) const { return m_color; }
            TEngine::Vec2 get_position(void) const { return m_pos; }
            TEngine::Vec2 get_min(void) const { return m_min; }
            TEngine::Vec2 get_max(void) const { return m_max; }
            int get_rotation_state(void) const { return m_prev_state * 10 + m_curr_state; }
            bool move(TEngine::Vec2 dir);
            bool move_position(TEngine::Vec2 pos);
            bool rotate(Rotation r);
            // grabs a copy of the blocks that make up the piece, with absolute positions for each, storing them on vec
            // vec has to be an empty vector
            // you may also just use the class' iterator (i.e. use the class instance in a for loop)
            size_t get_blocks(std::vector<TEngine::Vec2> &vec) const;
            // merely reads the blocks that make up the piece, with positions relative to the piece's
            const std::vector<TEngine::Vec2>& get_blocks(void) const { return m_body; }
        private:
            Type m_type;
            TEngine::Color m_color;
            TEngine::Vec2 m_pos;
            TEngine::Vec2 m_center;
            TEngine::Vec2 m_min;
            TEngine::Vec2 m_max;
            std::vector<TEngine::Vec2> m_body;
            // 0 = 0 -> spawn state
            // 1 = R
            // 2 = 2
            // 3 = L
            int m_prev_state;
            int m_curr_state;

            void update_bounds(TEngine::Vec2 r);
        public:
            class BlockIterator;
            BlockIterator begin() const { return Piece::BlockIterator{m_body.cbegin(), m_pos}; }
            BlockIterator end() const { return Piece::BlockIterator{m_body.cend(), m_pos}; }

            // not really necessary. just makes it a tiny little bit less annoying to render pieces on the screen
            class BlockIterator {
            private:
                std::vector<TEngine::Vec2>::const_iterator m_block_it;
                TEngine::Vec2 m_it_pos;
            public:
                BlockIterator(std::vector<TEngine::Vec2>::const_iterator it, TEngine::Vec2 it_pos) : m_block_it{it}, m_it_pos{it_pos} {}
                BlockIterator& operator++() { ++m_block_it; return *this; }
                BlockIterator operator++(int) { BlockIterator b = *this; ++(*this); return b; }
                bool operator==(BlockIterator other) { return m_block_it == other.m_block_it; }
                bool operator!=(BlockIterator other) { return m_block_it != other.m_block_it; }
                TEngine::Vec2 operator*() { TEngine::Vec2 r = *m_block_it; r += m_it_pos; return r; }

                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = std::vector<TEngine::Vec2>::const_iterator;
                using pointer = const std::vector<TEngine::Vec2>::const_iterator*;
                using reference = const std::vector<TEngine::Vec2>::const_iterator&;
                using iterator_category = std::input_iterator_tag;
            };
        };
        extern const std::unordered_map<Type, Piece> DEFAULT_PIECES;

        inline int get_amount_of_pieces(void) { return static_cast<int>(DEFAULT_PIECES.size()) - 2; /* two of the pieces are invalid */ }
        // absolutely prevent a copy
        inline const std::unordered_map<Type, Piece>& get_all_pieces() { return DEFAULT_PIECES; } 
        // prevent copy when a reference is all that's needed
        inline const Piece& get_piece(Type type) {
            if (type == Type::None) {
                // return Piece{};
            } else if (type == Type::Custom) {
                // return Piece{};
            }
            return DEFAULT_PIECES.at(type);
        }

        inline Rotation get_opposite_rotation(Rotation r) {
            if (r == Rotation::Clockwise) {
                return Rotation::Counterclockwise;
            } else if (r == Rotation::Counterclockwise) {
                return Rotation::Clockwise;
            } else {
                return Rotation::None;
            }
        }
    };
}