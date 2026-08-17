#include <unordered_map>
#include <array>

#include "tetris_game.hpp"
#include "tetris_utils.hpp"
#include "../engine/tengine.hpp"

// just to separate some things
namespace Tetris {
    namespace __Internal {
        namespace WallkickData {
            // wallkick tests 
            // some of the kick tests are the exact same for some rotations
            static constexpr std::array<TEngine::Vec2, 4> _0R = {TEngine::Vec2{-1,0},TEngine::Vec2{-1,1},TEngine::Vec2{0,-2},TEngine::Vec2{-1,-2}};
            static constexpr std::array<TEngine::Vec2, 4> _R0 = {TEngine::Vec2{1,0},TEngine::Vec2{1,-1},TEngine::Vec2{0,2},TEngine::Vec2{1,2}};
            static constexpr std::array<TEngine::Vec2, 4> _L0 = {TEngine::Vec2{-1,0},TEngine::Vec2{-1,-1},TEngine::Vec2{0,2},TEngine::Vec2{-1,2}};
            static constexpr std::array<TEngine::Vec2, 4> _0L = {TEngine::Vec2{1,0},TEngine::Vec2{1,1},TEngine::Vec2{0,-2},TEngine::Vec2{1,-2}};
            
            static constexpr std::array<TEngine::Vec2, 4> _I_0R = {TEngine::Vec2{-2,0},TEngine::Vec2{1,0},TEngine::Vec2{-2,-1},TEngine::Vec2{1,2}};
            static constexpr std::array<TEngine::Vec2, 4> _I_R0 = {TEngine::Vec2{2,0},TEngine::Vec2{-1,0},TEngine::Vec2{2,1},TEngine::Vec2{-1,-2}};
            static constexpr std::array<TEngine::Vec2, 4> _I_L0 = {TEngine::Vec2{1,0},TEngine::Vec2{-2,0},TEngine::Vec2{1,-2},TEngine::Vec2{-2,1}};
            static constexpr std::array<TEngine::Vec2, 4> _I_0L = {TEngine::Vec2{-1,0},TEngine::Vec2{2,0},TEngine::Vec2{-1,2},TEngine::Vec2{2,-1}};
            
            const std::unordered_map<int, std::array<TEngine::Vec2, 4>> WALLKICKS_ANY = {
                {TetrisUtils::rotstr_to_int("0->R"), _0R},
                {TetrisUtils::rotstr_to_int("R->2"), _R0}, 
                {TetrisUtils::rotstr_to_int("2->L"), _0L},
                {TetrisUtils::rotstr_to_int("L->0"), _L0},
                {TetrisUtils::rotstr_to_int("0->L"), _0L},
                {TetrisUtils::rotstr_to_int("L->2"), _L0},
                {TetrisUtils::rotstr_to_int("2->R"), _0R},
                {TetrisUtils::rotstr_to_int("R->0"), _R0} 
            };
            const std::unordered_map<int, std::array<TEngine::Vec2, 4>> WALLKICKS_I = {
                {TetrisUtils::rotstr_to_int("0->R"), _I_0R},
                {TetrisUtils::rotstr_to_int("R->2"), _I_0L},
                {TetrisUtils::rotstr_to_int("2->L"), _I_R0},
                {TetrisUtils::rotstr_to_int("L->0"), _I_L0},
                {TetrisUtils::rotstr_to_int("0->L"), _I_0L},
                {TetrisUtils::rotstr_to_int("L->2"), _I_0R},
                {TetrisUtils::rotstr_to_int("2->R"), _I_L0},
                {TetrisUtils::rotstr_to_int("R->0"), _I_R0}
            };
        }
   };
}