#pragma once

// #include <memory>
#include <cstdint>
#include "fonts.hpp"
#include "../texture_wrapper.hpp"

namespace TEngine::Text {
    // copying this struct will ONLY COPY THE POINTERS, NOT THE UNDERLYING DATA.
    // this struct has a destructor that frees the internal texture data.
    struct BitmapText {
        SDL_Renderer* renderer;
        TextureWrapper text_data;
    };

    namespace BitmapFontRenderer {
        // renders a single character to a BitmapText and returns it (by value)
        // containing the text rendered with the given font.
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_char(BitmapFont* f, char ch);
        // renders a (optionally) multi-line string to a BitmapText and returns it (by value)
        // containing the text rendered with the given font
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_str(BitmapFont* f, std::string_view str);
        // renders an integer to a BitmapText and returns it (by value)
        // containing the text rendered with the given font
        // BitmapText instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        BitmapText make_bitmap_text_num(BitmapFont* f, std::int64_t num);
        // this one might be better than render_char in terms of resource usage
        // since it just draws straight to the screen
        
        void draw_char(BitmapFont* f, char ch, float x, float y, float scale, float angle = 0, TextFlipMode flipmode = {});
        // it might be better to call make_bitmap_text_str() to cache the results or draw_string_line()
        // instead of this one. while the pre-processing might not necessarily be expensive,
        // it will be repeated every time the string is drawn, which might
        // become costly for drawing many (and bigger) strings
        void draw_string(BitmapFont* f, std::string_view str, float x, float y, float scale, float angle = 0, TextFlipMode flipmode = {});
        // draws a single string containing a single line straight to the screen using the font's
        // SDL_Renderer pointer
        void draw_string_line(BitmapFont* f, std::string_view str, float x, float y, float scale, float angle = 0, TextFlipMode flipmode = {});
        // draws a number straight to the screen using the font's SDL_Renderer pointer
        void draw_int64(BitmapFont* f, std::int64_t num, float x, float y, float scale, float angle = 0);
        void draw_float(BitmapFont* f, float num, float x, float y, float scale, float angle = 0);
        void draw_double(BitmapFont* f, double num, float x, float y, float scale, float angle = 0);
        
        void draw_char_color(BitmapFont* f, char ch, float x, float y, float scale, Color color, float angle = 0, TextFlipMode flipmode = {});
        void draw_string_color(BitmapFont* f, std::string_view str, float x, float y, float scale, Color color, float angle = 0, TextFlipMode flipmode = {});
        void draw_string_line_color(BitmapFont* f, std::string_view str, float x, float y, float scale, Color color, float angle = 0, TextFlipMode flipmode = {});
        
        void draw_int64_color(BitmapFont* f, std::int64_t num, float x, float y, float scale, Color color, float angle = 0);
        void draw_float_color(BitmapFont* f, float num, float x, float y, float scale, Color color, float angle = 0);
        void draw_double_color(BitmapFont* f, double num, float x, float y, float scale, Color color, float angle = 0);
    }
    
    inline void draw_bitmap_text(const BitmapText& text, const SDL_FRect& target_rect) {
        SDL_RenderTexture(text.renderer, text.text_data.get(), nullptr, &target_rect);
    }
}