#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "text_renderer.hpp"
#include "fonts.hpp"

static inline std::tuple<TEngine::Vec2, TEngine::Vec2> __gen_points_for_line(int line_len, float x, float y, float scale, float angle) {
    float rw = line_len * scale;
    float rh = scale;
    TEngine::Vec2 point1 = TEngine::Vec2{
        x,
        y + (rh / 2.0f)
    } + TEngine::Vec2{(scale) / 2, 0};
    TEngine::Vec2 point2 = TEngine::Vec2{
        x + rw,
        y + (rh / 2.0f)
    } - TEngine::Vec2{(scale) / 2, 0};
    TEngine::Vec2 center = point1 + (point2 - point1) * 0.5f;

    point1 -= center;
    point1 = point1.rotate(angle);
    point1 += center;

    point2 -= center;
    point2 = point2.rotate(angle);
    point2 += center;

    // usual direction will always be towards the right side of the screen
    // unless rotations or mirroring are applied
    return std::make_tuple(point1, point2);
};

template<typename Number, typename... Args> requires(std::is_arithmetic_v<Number>)
static inline void __draw_number(TEngine::Text::BitmapFont* f, Number num, float x, float y, float scale, float angle = 0, Args&&... args) {
    std::array<char, 64> num_buf{};
    std::fill(std::begin(num_buf), std::end(num_buf), 0);
    std::to_chars_result res = std::to_chars(num_buf.data(), num_buf.data() + num_buf.size(), num, std::forward<Args>(args)...);
    if (res.ec == std::errc::value_too_large) {
        std::fill(std::begin(num_buf), std::end(num_buf), 0);
    }

    // res.ptr points to the end of the converted string
    std::size_t len = static_cast<std::size_t>(res.ptr - num_buf.data());
    const auto actual_size = f->tile_size() * scale;
    auto [start_pos, end_pos] = __gen_points_for_line(static_cast<int>(len), x, y, actual_size, angle);
    // for (std::size_t i = 0; i < len; i++) {
    //     f->draw_char(num_buf[i], x + static_cast<float>(i) * scale * f->tile_size(), y, scale);
    // }
    
    TEngine::Vec2 mov = TEngine::Vec2{actual_size / 2.0f, actual_size / 2.0f};
    for (std::size_t i = 0; i < len; ++i) {
        // lerp!!
        float t = static_cast<float>(i) / static_cast<float>(len - 1);
        TEngine::Vec2 actual_pos = TEngine::lerpv2(start_pos, end_pos, t) - mov;
        f->draw_char(
            num_buf[i],
            actual_pos.x,
            actual_pos.y,
            scale, angle, TEngine::Text::TextFlipMode::None
        );
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
            f->draw_char(ch, 0, 0, 1);
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
            const auto render_aux = [f, str](int start, int end, float y_off){
                for (int i = start; i < end; ++i) {
                    f->draw_char(str[i], static_cast<float>(i - start) * f->tile_size(), y_off * f->tile_size(), 1.0f);
                }
            };
            float y_off = 0;
            std::size_t prev_i = 0;
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

            SDL_Texture* num_texture = SDL_CreateTexture(f->get_renderer(), SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET, static_cast<int>(len) * f->tile_size(), f->tile_size());
            SDL_SetTextureScaleMode(num_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(f->get_renderer(), num_texture);
            SDL_SetRenderDrawColor(f->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(f->get_renderer());

            for (std::size_t i = 0; i < len; i++) {
                f->draw_char(num_buf[i], static_cast<float>(i) * f->tile_size(), 0, 1);
            }

            SDL_SetRenderTarget(f->get_renderer(), nullptr);
            return {f->get_renderer(), num_texture};
        }

        Color set_text_color_mod(BitmapFont* f, Color color) {
            auto* texture = f->get_texture_atlas().get_texture().get();
            Color mod;
            SDL_GetTextureColorMod(texture, &mod.r, &mod.g, &mod.b);
            SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
            return mod;
        }
        
        // this one might be better than make_bitmap_text_ch in terms of resource usage
        // since it just draws straight to the screen
        void draw_char(BitmapFont* f, char ch, float x, float y, float scale, float angle, TextFlipMode flipmode) {
            f->draw_char(ch, x, y, scale, angle, flipmode);
        }
        
        // ok
        void draw_string(BitmapFont* f, std::string_view str, float x, float y, float scale, float angle, TextFlipMode flipmode) {
            // 1 - view-based solution (C++20)
            using namespace std::string_view_literals;
            // float y_off = 0;
            // for (const auto line : std::views::split(str, "\n"sv)) {
            //     // const auto data = std::string_view{line.begin(), line.end()};
            //     int len = static_cast<int>(line.size());
            //     for (int i = 0; i < len; ++i) {
            //         f->draw_char(
            //             line[i],
            //             x + static_cast<float>(i) * scale * f->tile_size(),
            //             y + y_off * scale * f->tile_size(),
            //             scale, angle, flipmode
            //         );
            //     }
            //     y_off += 1;
            // }

            int longest_line = -1;
            int total_lines = 0;
            for (const auto line : std::views::split(str, "\n"sv)) {
                longest_line = std::max(longest_line, static_cast<int>(line.size()));
                total_lines += 1;
            }
            const auto actual_size = f->tile_size() * scale;
            auto [start_pos, end_pos] = __gen_points_for_line(longest_line, x, y + ((total_lines - 1)/2.0f) * actual_size, actual_size, angle);

            // float line_off = -(static_cast<float>(total_lines) / 2.0f - 0.5f);
            const auto draw_full = [=](float line_offset, float line_inc){
                Vec2 mov = Vec2{actual_size / 2.0f, actual_size / 2.0f};
                for (const auto line : std::views::split(str, "\n"sv)) {
                    int len = static_cast<int>(line.size());
                    // for (int i = 0; i < len; ++i) {
                    //     float t = static_cast<float>(i) / static_cast<float>(longest_line - 1);
                    //     Vec2 cpos =
                    //         start_pos * (1 - t) + 
                    //         end_pos * t;
                    //     Vec2 up = (Vec2{0, 1} * (actual_size * line_off)).rotate(angle);
                    //     // Vec2 actual_pos = cpos + up - mov;
                    //     Vec2 actual_pos = lerpv2(start_pos, end_pos, t) - mov;
                    //     f->draw_char(
                    //         line[i],
                    //         actual_pos.x,
                    //         actual_pos.y,
                    //         scale, angle, flipmode
                    //     );
                    // }
                    const auto draw = [=](auto&& line, int starting_i){
                        int i = starting_i;
                        for (auto it = line.begin(); it != line.end(); ++it) {
                            float t = static_cast<float>(i) / static_cast<float>(longest_line - 1);
                            Vec2 up = (Vec2{0, 1} * (actual_size * line_offset)).rotate(angle);
                            Vec2 actual_pos = lerpv2(start_pos, end_pos, t) + up - mov;
                            f->draw_char(
                                *it,
                                actual_pos.x,
                                actual_pos.y,
                                scale, angle, flipmode
                            );
                            i++;
                        }
                    };
                    // draw(line.begin(), line.end());
                    if (flipmode == TextFlipMode::Horizontal || flipmode == TextFlipMode::Both) {
                        draw(line | std::views::reverse, longest_line - len);
                    } else {
                        draw(line, 0);
                    }
                    line_offset += line_inc;
                }
            };
            float line_off = (static_cast<float>(total_lines) / 2.0f - 0.5f);
            if (flipmode == TextFlipMode::Vertical || flipmode == TextFlipMode::Both) {
                draw_full(line_off, -1);
            } else {
                draw_full(-line_off, 1);
            }


            // // 2 - regular loop solution (C++17)
            // int prev_i = 0;
            // for (size_t i = 0; i < str.length(); ++i) {
            //     if (str[i] != '\n') {
            //         continue;
            //     }
            //     for (size_t j = prev_i; j < i; ++j) {
            //         f->draw_char(str[j], x + static_cast<float>(j - prev_i) * actual_size, y + y_off * actual_size, scale);
            //     }
            //     y_off += 1;
            //     prev_i = i + 1;
            // }
            // for (size_t j = prev_i; j < str.length(); ++j) {
            //     f->draw_char(str[j], x + static_cast<float>(j - prev_i) * actual_size, y + y_off * actual_size, scale);
            // }
        }

        void draw_string_line(BitmapFont* f, std::string_view line, float x, float y, float scale, float angle, TextFlipMode flipmode) {
            const auto actual_size = f->tile_size() * scale;
            // version that works fine, with only per-character rotations (keep it here just in case)
            // int len = static_cast<int>(line.length());
            // for (int i = 0; i < len; ++i) {
            //     f->draw_char(
            //         line[i],
            //         x + static_cast<float>(i) * actual_size,
            //         y,
            //         scale, angle, flipmode
            //     );
            // }
            int len = static_cast<int>(line.length());
            auto [start_pos, end_pos] = __gen_points_for_line(len, x, y, actual_size, angle);
            
            Vec2 mov = Vec2{actual_size / 2.0f, actual_size / 2.0f};
            // for (int i = 0; i < len; ++i) {
            //     // lerp!!
            //     float t = static_cast<float>(i) / static_cast<float>(len - 1);
            //     if (len == 1) t = 1;
            //     Vec2 cpos =
            //         start_pos * ((1 - t)) + 
            //         end_pos * t;
            //     Vec2 actual_pos = cpos - mov;
            //     f->draw_char(
            //         line[i],
            //         actual_pos.x,
            //         actual_pos.y,
            //         scale, angle, flipmode
            //     );
            // }

            const auto draw = [f, scale, angle, flipmode, start_pos, end_pos, mov, len](auto&& begin, auto&& end){
                int i = 0;
                for (auto it = begin; it != end; ++it) {
                    // lerp!!
                    float t = static_cast<float>(i) / static_cast<float>(len - 1);
                    if (len == 1) t = 1;
                    Vec2 actual_pos = lerpv2(start_pos, end_pos, t) - mov;
                    f->draw_char(
                        *it,
                        actual_pos.x,
                        actual_pos.y,
                        scale, angle, flipmode
                    );
                    i++;
                }
            };

            // if (static_cast<int>(flipmode) & static_cast<int>(TextFlipMode::Horizontal)) {
            if (flipmode == TextFlipMode::Horizontal || flipmode == TextFlipMode::Both) {
                draw(line.rbegin(), line.rend());
            } else {
                draw(line.begin(), line.end());
            }
        }

        // m e s s y y y y y y y !!!!!
        void draw_int64(BitmapFont* f, std::int64_t num, float x, float y, float scale, float angle) {
            __draw_number(f, num, x, y, scale, angle);
        }

        void draw_float(BitmapFont* f, float num, float x, float y, float scale, float angle) {
            __draw_number(f, num, x, y, scale, angle, std::chars_format::fixed);
        }

        void draw_double(BitmapFont* f, double num, float x, float y, float scale, float angle) {
            __draw_number(f, num, x, y, scale, angle, std::chars_format::fixed);
        }
    }
}