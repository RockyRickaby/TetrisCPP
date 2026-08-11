#pragma once

// #include <memory>
#include <string>
#include <sstream>
#include "fonts.hpp"

namespace TEngine::Text {
    struct BitmapText {
        SDL_Renderer* renderer;
        SDL_Texture* text_data;

        BitmapText(SDL_Renderer* r, SDL_Texture* txt) :
            renderer{r},
            text_data(txt)
        {}
    };

    // TODO - think about how to better handle this thing
    namespace BitmapRenderer {
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline BitmapText render_char(SDL_Renderer* renderer, char ch) {
            SDL_Texture* char_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(renderer, char_texture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(renderer);
            Font::instance(renderer).render_char(renderer, char_texture, ch, 0, 0);
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, char_texture};
            // return Font::instance().draw_char(ch);
        }
        
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline BitmapText render_string(SDL_Renderer* renderer, const std::string& str) {
            int lines = 1;
            // int width = str.length();
            int width = -1;
            for (char ch : str) {
                if (ch == '\n') {
                    lines += 1;
                }
            }
            std::istringstream inputstream;
            inputstream.str(str);
            for (std::string line; std::getline(inputstream, line);) {
                // const std::string& line = str;
                int len = static_cast<int>(line.length());
                if (len > width) {
                    width = len;
                }
            }
            SDL_Texture* string_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, width * 8, lines * 8);
            SDL_SetTextureScaleMode(string_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(renderer, string_texture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(renderer);
            inputstream.clear();
            inputstream.str(str);
            float y_off = 0;
            for (std::string line; std::getline(inputstream, line);) {
                // const std::string& line = str;
                int len = static_cast<int>(line.length());
                for (int i = 0; i < len; ++i) {
                    Font::instance(renderer).render_char(renderer, string_texture, line[i], static_cast<float>(i), y_off);
                }
                y_off += 1;
            }
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, string_texture};
        }

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline SDL_Texture* render_number(SDL_Renderer* renderer, int num) {
            return nullptr;
        }
        
        // this one might be better than render_char in terms of resource usage
        // since it just draws straight to the screen
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline void draw_char(SDL_Renderer* renderer, char ch, float x, float y, float scale) {
            Font::instance().render_char_r(renderer, ch, x, y, scale);
        }
        
        // it might be better to call render_string() to cache the results instead of this one. while the pre-processing might
        // not necessarily be expensive, it will be repeated every time the string is drawn, which might
        // become costly for drawing many (and bigger) strings
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline void draw_string(SDL_Renderer* renderer, const std::string& str, float x, float y, float scale) {
            int lines = 1;
            int width = -1;
            for (char ch : str) {
                if (ch == '\n') {
                    lines += 1;
                }
            }
            std::istringstream inputstream;
            inputstream.str(str);
            for (std::string line; std::getline(inputstream, line);) {
                // const std::string& line = str;
                int len = static_cast<int>(line.length());
                if (len > width) {
                    width = len;
                }
            }
            inputstream.clear();
            inputstream.str(str);
            float y_off = 0;
            for (std::string line; std::getline(inputstream, line);) {
                int len = static_cast<int>(line.length());
                for (int i = 0; i < len; ++i) {
                    Font::instance(renderer).render_char_r(renderer, line[i], x + static_cast<float>(i) * scale * 8, y + y_off * scale * 8, scale);
                }
                y_off += 1;
            }
        }

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        inline void draw_number(SDL_Renderer* renderer, int num, float x, float y, float scale) {
            //return nullptr;
        }

        void destroy_bitmap_text(BitmapText text);
    }

    // no need to free/destroy it.
    // you do have to destroy these though
    // void destroy_bitmap_renderer(BitmapRenderer* bmr);
}