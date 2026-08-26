#include <charconv>
#include <cstdlib>
#include <cstdint>
// #include <ranges>
#include <string_view>
#include <type_traits>
#include "text_renderer.hpp"
// #include "fonts.hpp"

template<typename Number, typename... Args> requires(std::is_arithmetic_v<Number>)
static void draw_number(TEngine::Text::BitmapFont* f, Number num, float x, float y, float scale, Args... args) {
    char num_buf[21]{};
    std::to_chars_result res = std::to_chars(num_buf, num_buf + 20, num, args...);
    num_buf[20] = '\0';
    // res.ptr points to the end of the converted string
    std::size_t len = static_cast<std::size_t>(res.ptr - num_buf);
    for (std::size_t i = 0; i < len; i++) {
        f->render_char(num_buf[i], x + static_cast<float>(i) * scale * f->tile_size(), y, scale);
    }
}

namespace TEngine::Text {
    namespace BitmapFontRenderer {
        BitmapText make_bitmap_text_ch(BitmapFont* f, char ch) {
            SDL_Texture* char_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, f->tile_size(), f->tile_size());
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), char_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());
            f->render_char(ch, 0, 0, 1);
            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return BitmapText{f->get_renderer(), char_texture};
        }
        
        // memory is now allocated only for the actual SDL_Texture.
        // O(2n) -> O(n) (relative to the input string, ofc)
        BitmapText make_bitmap_text_str(BitmapFont* f, std::string_view str) {
            int lines = 1;
            int width = -1;
            // a little copy to more easily grab the length of the last (or only) line
            std::string_view strview = str;
            size_t idx = str.find("\n");
            size_t prev = 0;
            int len = -1;
            while (idx != std::string::npos) {
                lines += 1;
                len = static_cast<int>(idx - prev);
                if (len > width) {
                    width = len;
                }
                prev = idx + 1;
                strview = strview.substr(prev);
                idx = str.find("\n", prev);
            }
            len = static_cast<int>(strview.length());
            if (len > width) {
                width = len;
            }

            SDL_Texture* string_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, width * f->tile_size(), lines * f->tile_size());
            SDL_SetTextureScaleMode(string_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), string_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());
            const auto render_aux = [f, str](int start, int end, int y_off){
                for (int i = start; i < end; ++i) {
                    f->render_char(str[i], static_cast<float>(i - start) * f->tile_size(), y_off * f->tile_size(), 1);
                }
            };
            float y_off = 0;
            int prev_i = 0;
            for (size_t i = 0; i < str.length(); ++i) {
                if (str[i] != '\n') {
                    continue;
                }
                render_aux(static_cast<int>(prev_i), static_cast<int>(i), y_off);
                y_off += 1;
                prev_i = i + 1;
            }
            render_aux(static_cast<int>(prev_i), static_cast<int>(str.length()), y_off);
            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return BitmapText{f->get_renderer(), string_texture};
        }

        BitmapText make_bitmap_text_num(BitmapFont* f, std::int64_t num) {
            char num_buf[32]{};
            std::to_chars_result res = std::to_chars(num_buf, num_buf + 20, num, 10);
            num_buf[31] = '\0';
            // res.ptr points to the end of the converted string
            std::size_t len = static_cast<std::size_t>(res.ptr - num_buf);

            SDL_Texture* num_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, len * f->tile_size(), f->tile_size());
            SDL_SetTextureScaleMode(num_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), num_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());

            for (std::size_t i = 0; i < len; i++) {
                f->render_char(num_buf[i], static_cast<float>(i) * f->tile_size(), 0, 1);
            }

            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return {f->get_renderer(), num_texture};
        }
        
        // this one might be better than make_bitmap_text_ch in terms of resource usage
        // since it just draws straight to the screen
        void draw_char(BitmapFont* f, char ch, float x, float y, float scale) {
            f->render_char(ch, x, y, scale);
        }
        
        // ok
        void draw_string(BitmapFont* f, std::string_view str, float x, float y, float scale) {
            float y_off = 0;
            // 1 - view-based solution (C++20)
            // using namespace std::string_view_literals;
            // for (const auto line : std::views::split(str, "\n"sv)) {
            //     // const auto data = std::string_view{line.begin(), line.end()};
            //     int len = static_cast<int>(line.end() - line.begin());
            //     for (int i = 0; i < len; ++i) {
            //         f->render_char(line[i], x + static_cast<float>(i) * scale * f->tile_size(), y + y_off * scale * f->tile_size(), scale);
            //     }
            //     y_off += 1;
            // }

            // 2 - regular loop solution (C++17 because of the string_view,
            // but could be made to work on regular strings as well, making it compatible with previous versions)
            int prev_i = 0;
            for (size_t i = 0; i < str.length(); ++i) {
                if (str[i] != '\n') {
                    continue;
                }
                for (size_t j = prev_i; j < i; ++j) {
                    f->render_char(str[j], x + static_cast<float>(j - prev_i) * scale * f->tile_size(), y + y_off * scale * f->tile_size(), scale);
                }
                y_off += 1;
                prev_i = i + 1;
            }
            for (size_t j = prev_i; j < str.length(); ++j) {
                f->render_char(str[j], x + static_cast<float>(j - prev_i) * scale * f->tile_size(), y + y_off * scale * f->tile_size(), scale);
            }
        }

        void draw_string_line(BitmapFont* f, std::string_view line, float x, float y, float scale) {
            int len = static_cast<int>(line.length());
            for (int i = 0; i < len; ++i) {
                f->render_char(line[i], x + static_cast<float>(i) * scale * f->tile_size(), y, scale);
            }
        }

        void draw_int64(BitmapFont* f, std::int64_t num, float x, float y, float scale) {
            draw_number(f, num, x, y, scale, 10);
        }

        void draw_float(BitmapFont* f, float num, float x, float y, float scale) {
            draw_number(f, num, x, y, scale, std::chars_format::fixed);
        }

        void draw_double(BitmapFont* f, double num, float x, float y, float scale) {
            draw_number(f, num, x, y, scale, std::chars_format::fixed);
        }
    }
}