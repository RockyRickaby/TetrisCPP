#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <SDL3/SDL_rect.h>

#include "tetris.hpp"
#include "tetris_utils.hpp"

namespace Tetris {
    namespace Tetrimino {
        Piece::Piece(Type type, Color color, Vec2 pos, Vec2 center, std::vector<Block>&& body) :
            type{type},
            color{color},
            m_pos{pos},
            m_center{center},
            m_body(std::move(body))
        {
            for (Block& b : m_body) {
                b.color = color;
            }
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
            for (Block& b : m_body) {
                b -= m_center;
                if (r == Rotation::CLOCKWISE) {
                    b.pos = Vec2{ .x = b.pos.y, .y = -b.pos.x };
                } else if (r == Rotation::COUNTERCLOCKWISE) {
                    b.pos = Vec2{ .x = -b.pos.y, .y = b.pos.x };
                }
                b += m_center;
            }
            // wall kick callback () or handle it here?
            return true;
        }

        Vec2 Piece::get_position(void) const {
            return m_pos;
        }
            
        const std::vector<Block>& Piece::get_blocks(void) const {
            return m_body;
        }

        size_t Piece::get_blocks(std::vector<Block> &vec) const {
            vec.reserve(m_body.size());
            for (const Block &b : m_body) {
                vec.push_back(b + m_pos);
            }
            return vec.size();
        }

        Piece::BlockIterator Piece::begin() const { return Piece::BlockIterator{m_body.begin(), m_pos}; }
        Piece::BlockIterator Piece::end() const { return Piece::BlockIterator{m_body.end(), m_pos}; }

        Piece::BlockIterator::BlockIterator(std::vector<Block>::const_iterator it, Vec2 it_pos) : m_block_it{it}, m_it_pos{it_pos} {}
        Piece::BlockIterator& Piece::BlockIterator::operator++() { ++m_block_it; return *this; }
        Piece::BlockIterator Piece::BlockIterator::operator++(int) { BlockIterator b = *this; ++(*this); return b; }
        bool Piece::BlockIterator::operator==(BlockIterator other) { return m_block_it == other.m_block_it; }
        bool Piece::BlockIterator::operator!=(BlockIterator other) { return m_block_it != other.m_block_it; }
        Block Piece::BlockIterator::operator*() { Block r = *m_block_it; r += m_it_pos; return r; }

        const std::unordered_map<Type, Piece> DEFAULT_PIECES = {
            {Type::NONE, Piece{Type::NONE, {}, {}, {}}},
            {Type::CUSTOM, Piece{Type::NONE, {}, {}, {}}},
            {Type::I, Piece{Type::I, {TetrisUtils::from_hex("#00E6FE")}, {0, 0}, {1.5, 1.5}, { Block{0,2,{}}, Block{1,2,{}}, Block{2,2,{}}, Block{3,2,{}} }}}, 
            {Type::J, Piece{Type::J, {TetrisUtils::from_hex("#1801FF")}, {0, 0}, {1, 1}, { Block{0,2,{}}, Block{0,1,{}}, Block{1,1,{}}, Block{2,1,{}} }}}, 
            {Type::L, Piece{Type::L, {TetrisUtils::from_hex("#FF7308")}, {0, 0}, {1, 1}, { Block{0,1,{}}, Block{1,1,{}}, Block{2,1,{}}, Block{2,2,{}} }}}, 
            {Type::O, Piece{Type::O, {TetrisUtils::from_hex("#FFDE00")}, {0, 0}, {1.5, 1.5}, { Block{1,1,{}}, Block{2,1,{}}, Block{1,2,{}}, Block{2,2,{}} }}}, 
            {Type::S, Piece{Type::S, {TetrisUtils::from_hex("#66FD00")}, {0, 0}, {1, 1}, { Block{0,1,{}}, Block{1,1,{}}, Block{1,2,{}}, Block{2,2,{}} }}}, 
            {Type::Z, Piece{Type::Z, {TetrisUtils::from_hex("#FE103C")}, {0, 0}, {1, 1}, { Block{0,2,{}}, Block{1,2,{}}, Block{1,1,{}}, Block{2,1,{}} }}}, 
            {Type::T, Piece{Type::T, {TetrisUtils::from_hex("#B802FD")}, {0, 0}, {1, 1}, { Block{0,1,{}}, Block{1,1,{}}, Block{1,2,{}}, Block{2,1,{}} }}}
        };
    }
}