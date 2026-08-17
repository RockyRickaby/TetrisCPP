#pragma once

// #include <memory>
#include <string>
#include <cstdint>
#include "fonts.hpp"

namespace TEngine::Text {
    struct BitmapText {
        SDL_Renderer* renderer;
        SDL_Texture* text_data;   
    };

    namespace BitmapFontRenderer {
        // renders a single character to a BitmapText and returns it (by value)
        // containing the text rendered with the given font.
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_char(BitmapFonts::Font* f, char ch);
        // renders a (optionally) multi-line string to a BitmapText and returns it (by value)
        // containing the text rendered with the given font
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_str(BitmapFonts::Font* f, const std::string& str);
        // renders an integer to a BitmapText and returns it (by value)
        // containing the text rendered with the given font
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_num(BitmapFonts::Font* f, std::int64_t num);
        // this one might be better than render_char in terms of resource usage
        // since it just draws straight to the screen
        void draw_char(BitmapFonts::Font* f, char ch, float x, float y, float scale);
        // it might be better to call make_bitmap_text_str() to cache the results or draw_string_line()
        // instead of this one. while the pre-processing might not necessarily be expensive,
        // it will be repeated every time the string is drawn, which might
        // become costly for drawing many (and bigger) strings
        void draw_string(BitmapFonts::Font* f, const std::string& str, float x, float y, float scale);
        // draws a single string containing a single line straight to the screen using the font's
        // SDL_Renderer pointer
        void draw_string_line(BitmapFonts::Font* f, const std::string& str, float x, float y, float scale);
        // draws a number straight to the screen using the font's SDL_Renderer pointer
        void draw_int64(BitmapFonts::Font* f, std::int64_t num, float x, float y, float scale);
        // void draw_float(BitmapFonts::Font* f, float num, float x, float y, float scale);
    }
    // destroys the text's internal data
    void destroy_bitmap_text(BitmapText text);
    // draws the BitmapText to the screen at the position and with the scale given by target_rect
    void draw_bitmap_text(const BitmapText& text, const SDL_FRect& target_rect);
}