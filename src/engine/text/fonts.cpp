#include <SDL3/SDL_surface.h>
#include <vector>
#include <SDL3/SDL.h>

#include "fonts.hpp"

namespace TEngine::Text {
    JoustFont::JoustFont(const std::filesystem::path& font_path, int tile_size, SDL_Renderer* renderer, Color color_key) :
        m_atlas{font_path, tile_size, tile_size, renderer, SDL_SCALEMODE_NEAREST, color_key}
    {
        setup_atlas();
    }

    void JoustFont::render_char(char c, float offset_x, float offset_y, float scale) {
        m_atlas.render(static_cast<int>(c), offset_x, offset_y, scale);
    }

    void JoustFont::setup_atlas() {
       std::vector<std::pair<char, int>> m_char_to_idx{{
            {'A', 0},{'a', 0},
            {'B', 1},{'b', 1},
            {'C', 2},{'c', 2},
            {'D', 3},{'d', 3},
            {'E', 4},{'e', 4},
            {'F', 5},{'f', 5},
            {'G', 6},{'g', 6},
            {'H', 7},{'h', 7},
            {'I', 8},{'i', 8},
            {'J', 9},{'j', 9},
            {'K', 10},{'k', 10},
            {'L', 11},{'l', 11},
            {'M', 12},{'m', 12},
            {'N', 13},{'n', 13},
            {'O', 14},{'o', 14},
            {'P', 15},{'p', 15},
            {'Q', 16},{'q', 16},
            {'R', 17},{'r', 17},
            {'S', 18},{'s', 18},
            {'T', 19},{'t', 19},
            {'U', 20},{'u', 20},
            {'V', 21},{'v', 21},
            {'W', 22},{'w', 22},
            {'X', 23},{'x', 23},
            {'Y', 24},{'y', 24},
            {'Z', 25},{'z', 25},
            {'0', 26},
            {'1', 27},
            {'2', 28},
            {'3', 29},
            {'4', 30},
            {'5', 31},
            {'6', 32},
            {'7', 33},
            {'8', 34},
            {'9', 35},
            {'?', 36},
            {'(', 39},
            {'-', 40},
            {'.', 41},
            {'=', 44},
            {'!', 45},
            {')', 46},
            {' ', 47} // blank square
        }};

        for (const auto [ch, idx] : m_char_to_idx) {
            m_atlas.insert_offsets(static_cast<int>(ch), idx % 10, idx / 10);
        }
    }
}