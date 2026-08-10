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
    class BitmapRenderer {
    public:
        // no need for custom destroyer, as no memory should be managed here
        BitmapRenderer(SDL_Renderer* r = nullptr) : renderer{r} {}

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        BitmapText render_char(char ch) {
            SDL_Texture* char_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(renderer, char_texture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(renderer);
            Font::instance().render_char(renderer, char_texture, ch, 0, 0);
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, char_texture};
            // return Font::instance().draw_char(ch);
        }
        
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        BitmapText render_string(const std::string& str) {
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
                    Font::instance().render_char(renderer, string_texture, line[i], static_cast<float>(i), y_off);
                }
                y_off += 1;
            }
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, string_texture};
        }

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        SDL_Texture* render_number(int num) {
            return nullptr;
        }

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        BitmapText draw_char(char ch, float x, float y, float scale) {
            SDL_Texture* char_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(renderer, char_texture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(renderer);
            Font::instance().render_char(renderer, char_texture, ch, 0, 0);
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, char_texture};
            // return Font::instance().render_char(ch);
        }
        
        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        BitmapText draw_string(const std::string& str, float x, float y, float scale) {
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
                    Font::instance().render_char(renderer, string_texture, line[i], static_cast<float>(i), y_off);
                }
                y_off += 1;
            }
            SDL_SetRenderTarget(renderer, nullptr);
            return BitmapText{renderer, string_texture};
        }

        template<typename Font> requires(std::is_base_of_v<BitmapFonts::Font, Font>)
        SDL_Texture* draw_number(int num, float x, float y, float scale) {
            return nullptr;
        }
    private:
        SDL_Renderer* renderer;
    };

    // no need to free/destroy it.
    BitmapRenderer init_bitmap_renderer(SDL_Renderer* renderer);
    // you do have to destroy these though
    void destroy_bitmap_text(BitmapText text);
    // void destroy_bitmap_renderer(BitmapRenderer* bmr);
}