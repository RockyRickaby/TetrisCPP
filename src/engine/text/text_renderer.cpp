#include <cstdlib>
#include <sstream>
#include <cstdint>
#include "text_renderer.hpp"
// #include "fonts.hpp"

namespace TEngine::Text {
    // void destroy_bitmap_renderer(BitmapRenderer* bmr) {
    //     // delete bmr;
    // }
    namespace BitmapFontRenderer {
        BitmapText make_bitmap_text_ch(BitmapFonts::Font* f, char ch) {
            SDL_Texture* char_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, f->tile_size(), f->tile_size());
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), char_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());
            f->render_char_texture(ch, 0, 0);
            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return BitmapText{f->get_renderer(), char_texture};
        }
        
        BitmapText make_bitmap_text_str(BitmapFonts::Font* f, const std::string& str) {
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
            SDL_Texture* string_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, width * f->tile_size(), lines * f->tile_size());
            SDL_SetTextureScaleMode(string_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), string_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());
            inputstream.clear();
            inputstream.str(str);
            float y_off = 0;
            for (std::string line; std::getline(inputstream, line);) {
                // const std::string& line = str;
                int len = static_cast<int>(line.length());
                for (int i = 0; i < len; ++i) {
                    f->render_char_texture(line[i], static_cast<float>(i), y_off);
                }
                y_off += 1;
            }
            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return BitmapText{f->get_renderer(), string_texture};
        }

        BitmapText make_bitmap_text_num(BitmapFonts::Font* f, std::int64_t num) {
            std::string num_str = std::to_string(num);
            int len = static_cast<int>(num_str.length()); 

            SDL_Texture* num_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, len * f->tile_size(), f->tile_size());
            SDL_SetTextureScaleMode(num_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), num_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());

            for (std::size_t i = 0; i < len; i++) {
                f->render_char_texture(num_str[i], static_cast<float>(i), 0);
            }

            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return {f->get_renderer(), num_texture};
        }
        
        // this one might be better than make_bitmap_text_ch in terms of resource usage
        // since it just draws straight to the screen
        void draw_char(BitmapFonts::Font* f, char ch, float x, float y, float scale) {
            f->render_char_renderer(ch, x, y, scale);
        }
        
        // it might be better to call render_string() to cache the results instead of this one. while the pre-processing might
        // not necessarily be expensive, it will be repeated every time the string is drawn, which might
        // become costly for drawing many (and bigger) strings
        void draw__string(BitmapFonts::Font* f, const std::string& str, float x, float y, float scale) {
            int width = -1;
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
                    f->render_char_renderer(line[i], x + static_cast<float>(i) * scale * f->tile_size(), y + y_off * scale * f->tile_size(), scale);
                }
                y_off += 1;
            }
        }

        void draw_string_line(BitmapFonts::Font* f, const std::string& line, float x, float y, float scale) {
            int len = static_cast<int>(line.length());
            for (int i = 0; i < len; ++i) {
                f->render_char_renderer(line[i], x + static_cast<float>(i) * scale * f->tile_size(), y, scale);
            }
        }

        void draw_int64(BitmapFonts::Font* f, std::int64_t num, float x, float y, float scale) {
            std::string num_str = std::to_string(num);
            std::size_t len = num_str.length(); 
            for (std::size_t i = 0; i < len; i++) {
                f->render_char_renderer(num_str[i], x + static_cast<float>(i) * scale * f->tile_size(), y, scale);
            }
        }
    }

    void destroy_bitmap_text(BitmapText text) {
        SDL_DestroyTexture(text.text_data);
    }

    void draw_bitmap_text(const BitmapText& text, const SDL_FRect& target_rect) {
        SDL_RenderTexture(text.renderer, text.text_data, nullptr, &target_rect);
    }
}