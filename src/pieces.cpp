#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <SDL3/SDL_rect.h>

#include "tetris_base.hpp"
#include "tetris_utils.hpp"

namespace Tetris {
    namespace Tetrimino {
        Piece::Piece() :
            type{},
            color{},
            m_pos{},
            m_center{},
            m_min{},
            m_max{},
            prev_state{},
            curr_state{}
        {}

        Piece::Piece(Type type, Color color, Vec2 pos, Vec2 center, std::vector<Vec2>&& body) :
            type{type},
            color{color},
            m_pos{pos},
            m_center{center},
            m_min{0xFFFF,0xFFFF},
            m_max{-0xFFFF,-0xFFFF},
            m_body(std::move(body)),
            prev_state{0},
            curr_state{0}
        {
            for (Vec2& v : m_body) {
                if (v.x > m_max.x) {
                    m_max.x = v.x;
                }
                if (v.y > m_max.y) {
                    m_max.y = v.y;
                }
                if (v.x < m_min.x) {
                    m_min.x = v.x;
                }
                if (v.y < m_min.y) {
                    m_min.y = v.y;
                }
            }
        }

        Type Piece::get_type(void) const {
            return type;
        }
        Color Piece::get_color(void) const {
            return color;
        }
        Vec2 Piece::get_position(void) const {
            return m_pos;
        }
        Vec2 Piece::get_min(void) const {
            return m_min;
        }
        Vec2 Piece::get_max(void) const {
            return m_max;
        }
        int Piece::get_rotation_state(void) const {
            return prev_state * 10 + curr_state;
        }

        bool Piece::move(Vec2 dir) {
            m_pos += dir;
            return true;
        }

        bool Piece::move_position(Vec2 pos) {
            m_pos = pos;
            return true;
        }

        bool Piece::rotate(Rotation r) {
            Vec2 v_r{0,0};
            constexpr int max_states = 4;
            if (r == Rotation::NONE) {
                return true;
            } else if (r == Rotation::CLOCKWISE) { // (y,-x)
                v_r.x = 1;
                v_r.y = -1;

                prev_state = curr_state;
                curr_state = (curr_state + 1 + max_states) % max_states;
            } else if (r == Rotation::COUNTERCLOCKWISE) { // (-y,x)
                v_r.x = -1;
                v_r.y = 1;
                
                prev_state = curr_state;
                curr_state = (curr_state - 1 + max_states) % max_states;
            }

            for (Vec2& v : m_body) {
                v -= m_center;
                v = Vec2{ .x = v_r.x * v.y, .y = v_r.y * v.x };
                v += m_center;
            }
            update_bounds(v_r);
            return true;
        }
            
        const std::vector<Vec2>& Piece::get_blocks(void) const {
            return m_body;
        }

        size_t Piece::get_blocks(std::vector<Vec2> &vec) const {
            vec.reserve(m_body.size());
            size_t count = 0;
            for (const Vec2 &v : m_body) {
                vec.push_back(v + m_pos);
                count += 1;
            }
            return count;
        }

        void Piece::update_bounds(Vec2 r) {
            m_min -= m_center;
            m_min = Vec2{ .x = r.x * m_min.y, .y = r.y * m_min.x };
            m_min += m_center;

            m_max -= m_center;
            m_max = Vec2{ .x = r.x * m_max.y, .y = r.y * m_max.x };
            m_max += m_center;

            if (m_max.x < m_min.x) {
                std::swap(m_max.x, m_min.x);
            }
            if (m_max.y < m_min.y) {
                std::swap(m_max.y, m_min.y);
            }
        }

        Piece::BlockIterator Piece::begin() const { return Piece::BlockIterator{m_body.begin(), m_pos}; }
        Piece::BlockIterator Piece::end() const { return Piece::BlockIterator{m_body.end(), m_pos}; }

        Piece::BlockIterator::BlockIterator(std::vector<Vec2>::const_iterator it, Vec2 it_pos) : m_block_it{it}, m_it_pos{it_pos} {}
        Piece::BlockIterator& Piece::BlockIterator::operator++() { ++m_block_it; return *this; }
        Piece::BlockIterator Piece::BlockIterator::operator++(int) { BlockIterator b = *this; ++(*this); return b; }
        bool Piece::BlockIterator::operator==(BlockIterator other) { return m_block_it == other.m_block_it; }
        bool Piece::BlockIterator::operator!=(BlockIterator other) { return m_block_it != other.m_block_it; }
        Vec2 Piece::BlockIterator::operator*() { Vec2 r = *m_block_it; r += m_it_pos; return r; }

        const std::unordered_map<Type, Piece> DEFAULT_PIECES = {
            {Type::NONE, Piece{}},
            {Type::CUSTOM, Piece{}},
            {Type::I, Piece{Type::I, {TetrisUtils::color_from_hex("#00E6FE")}, {3, 20}, {1.5, -0.5}, { Vec2{0,0}, Vec2{1,0}, Vec2{2,0}, Vec2{3,0} }}}, 
            {Type::J, Piece{Type::J, {TetrisUtils::color_from_hex("#1801FF")}, {4, 20}, {0, 0}, { Vec2{-1,1}, Vec2{-1,0}, Vec2{0,0}, Vec2{1,0} }}}, 
            {Type::L, Piece{Type::L, {TetrisUtils::color_from_hex("#FF7308")}, {4, 20}, {0, 0}, { Vec2{1,0}, Vec2{0,0}, Vec2{-1,0}, Vec2{1,1} }}},
            {Type::O, Piece{Type::O, {TetrisUtils::color_from_hex("#FFDE00")}, {4, 20}, {0.5, 0.5}, { Vec2{0,0}, Vec2{1,1}, Vec2{1,0}, Vec2{0,1} }}}, 
            {Type::S, Piece{Type::S, {TetrisUtils::color_from_hex("#66FD00")}, {4, 20}, {0, 0}, { Vec2{0,0}, Vec2{-1,0}, Vec2{0,1}, Vec2{1,1} }}},
            {Type::Z, Piece{Type::Z, {TetrisUtils::color_from_hex("#FE103C")}, {4, 20}, {0, 0}, { Vec2{0,0}, Vec2{1,0}, Vec2{0,1}, Vec2{-1,1} }}},
            {Type::T, Piece{Type::T, {TetrisUtils::color_from_hex("#B802FD")}, {4, 20}, {0, 0}, { Vec2{0,0}, Vec2{1,0}, Vec2{-1,0}, Vec2{0,1} }}}
        };

        Piece get_piece(Type type) {
            return DEFAULT_PIECES.at(type);
        }

        Rotation get_opposite_rotation(Rotation r) {
            if (r == Rotation::CLOCKWISE) {
                return Rotation::COUNTERCLOCKWISE;
            } else if (r == Rotation::COUNTERCLOCKWISE) {
                return Rotation::CLOCKWISE;
            } else {
                return Rotation::NONE;
            }
        }
    }
}