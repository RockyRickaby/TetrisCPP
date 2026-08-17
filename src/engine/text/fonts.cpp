#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <SDL3/SDL.h>

#include "fonts.hpp"

namespace TEngine::Text::BitmapFonts {
    JoustFont::JoustFont(SDL_Renderer* renderer, const char* font_path) :
        m_renderer{renderer}
    {
        SDL_Surface* img = SDL_LoadPNG(font_path);
        if (!img) {
            std::clog << "JoustFont(const char*): could not load font image\n";
            std::clog << SDL_GetError();
            std::abort();
        }
        std::uint32_t key = SDL_MapRGB(SDL_GetPixelFormatDetails(img->format), nullptr, 0, 0, 0);
        SDL_SetSurfaceColorKey(img, true, key);
        SDL_Texture* img_txt = SDL_CreateTextureFromSurface(renderer, img);
        if (!img_txt) {
            std::clog << "JoustFont(const char*): could not create texture from image\n";
            std::clog << SDL_GetError() << '\n';
            std::abort();
        }
        SDL_SetTextureScaleMode(img_txt, SDL_SCALEMODE_NEAREST);
        m_font_map = img_txt;
        SDL_DestroySurface(img);
        setup_map();
    }

    JoustFont::~JoustFont() {
        SDL_DestroyTexture(m_font_map);
    }

    void JoustFont::render_char_texture(char c, float offset_x, float offset_y) {
        const auto idx_to_pair = [this](char ch) -> std::pair<float, float> {
            int idx = -1;
            if (m_char_to_idx.contains(ch)) {
                idx = m_char_to_idx.at(ch);
            } else {
                idx = m_char_to_idx.at(' ');
            }

            int x = idx % 10;
            int y = idx / 10;
            return std::make_pair<float, float>(x * 8, y * 8);
        };
        auto [x, y] = idx_to_pair(c);
        SDL_FRect source = { .x = x, .y = y, .w = 8, .h = 8 };
        SDL_FRect dest = { .x = offset_x * 8, .y = offset_y * 8, .w = 8, .h = 8 };
        SDL_RenderTexture(m_renderer, m_font_map, &source, &dest);
    }

    void JoustFont::render_char_renderer(char c, float offset_x, float offset_y, float scale) {
        const auto idx_to_pair = [this](char ch) -> std::pair<float, float> {
            int idx = -1;
            if (m_char_to_idx.contains(ch)) {
                idx = m_char_to_idx.at(ch);
            }
            else {
                idx = m_char_to_idx.at(' ');
            }

            int x = idx % 10;
            int y = idx / 10;
            return std::make_pair<float, float>(x * 8, y * 8);
            };
        auto [x, y] = idx_to_pair(c);
        SDL_FRect source = { .x = x, .y = y, .w = 8, .h = 8 };
        SDL_FRect dest = { .x = offset_x, .y = offset_y, .w = 8 * scale, .h = 8 * scale };
        SDL_RenderTexture(m_renderer, m_font_map, &source, &dest);
    }

    void JoustFont::setup_map() {
        m_char_to_idx.insert({
            {'A', 0},
            {'B', 1},
            {'C', 2},
            {'D', 3},
            {'E', 4},
            {'F', 5},
            {'G', 6},
            {'H', 7},
            {'I', 8},
            {'J', 9},
            {'K', 10},
            {'L', 11},
            {'M', 12},
            {'N', 13},
            {'O', 14},
            {'P', 15},
            {'Q', 16},
            {'R', 17},
            {'S', 18},
            {'T', 19},
            {'U', 20},
            {'V', 21},
            {'W', 22},
            {'X', 23},
            {'Y', 24},
            {'Z', 25},
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
        });
    }
}