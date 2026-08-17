#include <vector>
#include <algorithm>
#include <unordered_map>
// #include <map>
#include <type_traits>
#include <SDL3/SDL_rect.h>

#include "tetris_pieces.hpp"
#include "../engine/utils.hpp"

namespace Tetris {
    namespace Tetrimino {
        Piece::Piece() :
            m_type{},
            m_color{},
            m_pos{},
            m_center{},
            m_min{},
            m_max{},
            m_prev_state{},
            m_curr_state{}
        {}

        Piece::Piece(Type type, TEngine::Color color, TEngine::Vec2 pos, TEngine::Vec2 center, std::vector<TEngine::Vec2>&& body) :
            m_type{type},
            m_color{color},
            m_pos{pos},
            m_center{center},
            m_min{0xFFFF,0xFFFF},
            m_max{-0xFFFF,-0xFFFF},
            m_body(std::move(body)),
            m_prev_state{0},
            m_curr_state{0}
        {
            for (TEngine::Vec2& v : m_body) {
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

        bool Piece::move(TEngine::Vec2 dir) {
            m_pos += dir;
            return true;
        }

        bool Piece::move_position(TEngine::Vec2 pos) {
            m_pos = pos;
            return true;
        }

        bool Piece::rotate(Rotation r) {
            TEngine::Vec2 v_r{0,0};
            constexpr int max_states = 4;
            if (r == Rotation::None) {
                return true;
            } else if (r == Rotation::Clockwise) { // (y,-x)
                v_r.x = 1;
                v_r.y = -1;

                m_prev_state = m_curr_state;
                m_curr_state = (m_curr_state + 1 + max_states) % max_states;
            } else if (r == Rotation::Counterclockwise) { // (-y,x)
                v_r.x = -1;
                v_r.y = 1;
                
                m_prev_state = m_curr_state;
                m_curr_state = (m_curr_state - 1 + max_states) % max_states;
            }

            for (TEngine::Vec2& v : m_body) {
                v -= m_center;
                v = TEngine::Vec2{ .x = v_r.x * v.y, .y = v_r.y * v.x };
                v += m_center;
            }
            update_bounds(v_r);
            return true;
        }

        size_t Piece::get_blocks(std::vector<TEngine::Vec2> &vec) const {
            vec.reserve(m_body.size());
            size_t count = 0;
            for (const TEngine::Vec2 &v : m_body) {
                vec.push_back(v + m_pos);
                count += 1;
            }
            return count;
        }

        void Piece::update_bounds(TEngine::Vec2 r) {
            m_min -= m_center;
            m_min = TEngine::Vec2{ .x = r.x * m_min.y, .y = r.y * m_min.x };
            m_min += m_center;

            m_max -= m_center;
            m_max = TEngine::Vec2{ .x = r.x * m_max.y, .y = r.y * m_max.x };
            m_max += m_center;

            if (m_max.x < m_min.x) {
                std::swap(m_max.x, m_min.x);
            }
            if (m_max.y < m_min.y) {
                std::swap(m_max.y, m_min.y);
            }
        }

        const std::unordered_map<Type, Piece> DEFAULT_PIECES = {
        // const std::map<Type, Piece> DEFAULT_PIECES = {
            {Type::None, {}},
            {Type::Custom, {}},
            {Type::I, Piece{Type::I, {TEngine::Utils::color_from_hex("#00E6FE")}, {3, 20}, {1.5, -0.5}, { TEngine::Vec2{0,0}, TEngine::Vec2{1,0}, TEngine::Vec2{2,0}, TEngine::Vec2{3,0} }}}, 
            {Type::J, Piece{Type::J, {TEngine::Utils::color_from_hex("#1801FF")}, {4, 20}, {0, 0}, { TEngine::Vec2{-1,1}, TEngine::Vec2{-1,0}, TEngine::Vec2{0,0}, TEngine::Vec2{1,0} }}}, 
            {Type::L, Piece{Type::L, {TEngine::Utils::color_from_hex("#FF7308")}, {4, 20}, {0, 0}, { TEngine::Vec2{1,0}, TEngine::Vec2{0,0}, TEngine::Vec2{-1,0}, TEngine::Vec2{1,1} }}},
            {Type::O, Piece{Type::O, {TEngine::Utils::color_from_hex("#FFDE00")}, {4, 20}, {0.5, 0.5}, { TEngine::Vec2{0,0}, TEngine::Vec2{1,1}, TEngine::Vec2{1,0}, TEngine::Vec2{0,1} }}}, 
            {Type::S, Piece{Type::S, {TEngine::Utils::color_from_hex("#66FD00")}, {4, 20}, {0, 0}, { TEngine::Vec2{0,0}, TEngine::Vec2{-1,0}, TEngine::Vec2{0,1}, TEngine::Vec2{1,1} }}},
            {Type::Z, Piece{Type::Z, {TEngine::Utils::color_from_hex("#FE103C")}, {4, 20}, {0, 0}, { TEngine::Vec2{0,0}, TEngine::Vec2{1,0}, TEngine::Vec2{0,1}, TEngine::Vec2{-1,1} }}},
            {Type::T, Piece{Type::T, {TEngine::Utils::color_from_hex("#B802FD")}, {4, 20}, {0, 0}, { TEngine::Vec2{0,0}, TEngine::Vec2{1,0}, TEngine::Vec2{-1,0}, TEngine::Vec2{0,1} }}}
        };
    }
}

static_assert(std::is_copy_assignable<Tetris::Tetrimino::Piece>::value);
static_assert(std::is_move_assignable<Tetris::Tetrimino::Piece>::value);