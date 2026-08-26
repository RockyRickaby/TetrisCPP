#pragma once

#include <cstdint>
#include <cmath>
#include <string_view>
#include "tengine.hpp"

namespace TEngine::TUtils {
    // uppercase letters stay uppercased
    // written for that compile-time evaluation
    // anything else that is not a letter may or may not change to something else completely unrelated
    inline constexpr unsigned char to_uppercase(unsigned char h) {
        // 6th bit changes from 1 (lowercase) to 0 (uppercase) (or remains unchanged if it's already 0)
        // ex.: 65 & ~(1 << 5) == 65, 65 == 'A'
        // ex.: 97 & ~(1 << 5) == 65, 97 == 'a' && 65 == 'A'
        return (h & ~(1 << 5));
    }

    // returns the base 10 value correspondent to a single hex digit
    // assumes that h is within the ranges [0-9], [a-f] or [A-F]
    // results not defined otherwise
    inline constexpr std::uint8_t hex_char_to_int(char h) {
        return h >= '0' && h <= '9' ? h - '0' : to_uppercase(static_cast<unsigned char>(h)) - 'A' + 10;
    }

    // the returned value is in the format RGBA8888
    inline constexpr std::uint32_t color_to_int32(Color c) {
        return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a; 
    }

    // assumes the argument is in the format RGBA8888
    inline constexpr Color color_from_uint32(std::uint32_t c) {
        return Color{
            .r = static_cast<std::uint8_t>((c >> 24) & 0xFF),
            .g = static_cast<std::uint8_t>((c >> 16) & 0xFF),
            .b = static_cast<std::uint8_t>((c >> 8) & 0xFF),
            .a = static_cast<std::uint8_t>(c & 0xFF) 
        }; 
    }

    // assumes the format #RRGGBB. alpha is set to 255 by default
    inline constexpr Color color_from_hex(std::string_view hex, std::uint8_t alpha = 255U) {
        // uint32_t color = std::stoi(hex, nullptr, 16);
        if (hex[0] == '#') {
            hex = hex.substr(1);
        }
        return Color{
            .r = static_cast<std::uint8_t>(hex_char_to_int(hex[0]) * 16 + hex_char_to_int(hex[1])),
            .g = static_cast<std::uint8_t>(hex_char_to_int(hex[2]) * 16 + hex_char_to_int(hex[3])),
            .b = static_cast<std::uint8_t>(hex_char_to_int(hex[4]) * 16 + hex_char_to_int(hex[5])),
            .a = alpha
        };
    }

    // alpha is preserved, but converted from [0,255] to [0,1]
    inline constexpr ColorHSB rgb_to_hsb(Color c) {
        float r = c.r / 255.0f;
        float g = c.g / 255.0f;
        float b = c.b / 255.0f;
        float cmax = std::fmax(std::fmax(r, g), b);
        float cmin = std::fmin(std::fmin(r, g), b);
        float delta = cmax - cmin;

        float hue = 0;
        float sat = cmax == 0.0f ? 0 : delta / cmax;
        float bri = cmax;
        if (delta != 0) {
            if (cmax == r) {
                hue = (g - b) / delta;
            } else if (cmax == g) {
                hue = 2 + (b - r) / delta;
            } else if (cmax == b) {
                hue = 4 + (r - g) / delta;
            }
            hue *= 60;
            if (hue < 0) {
                hue += 360;
            }
        }
        return {
            hue,
            sat,
            bri,
            c.a / 255.0f
        };
    }

    inline constexpr ColorHSB hex_to_hsb(std::string_view hex, std::uint8_t alpha = 255) {
        return rgb_to_hsb(color_from_hex(hex, alpha));
    }

    // alpha is preserved
    inline constexpr Color hsb_to_rgb(const ColorHSB& c) {
        float r, g, b;
        float hue = c.hue;
        if (c.sat == 0) {
            r = c.bri;
            g = c.bri;
            b = c.bri;
        } else {
            hue /= 60;
            int i = static_cast<int>(hue);
            float f = hue - i;
            float p = c.bri * (1 - c.sat);
            float q = c.bri * (1 - c.sat * f);
            float t = c.bri * (1 - c.sat * (1 - f));
            switch(i) {
                case 0: {
                    r = c.bri;
                    g = t;
                    b = p;
                } break;

                case 1: {
                    r = q;
                    g = c.bri;
                    b = p;
                } break;

                case 2: {
                    r = p;
                    g = c.bri;
                    b = t;
                } break;

                case 3: {
                    r = p;
                    g = q;
                    b = c.bri;
                } break;

                case 4: {
                    r = t;
                    g = p;
                    b = c.bri;
                } break;

                case 5:
                default: {
                    r = c.bri;
                    g = p;
                    b = q;
                } break;
            }
        }
        return {
            static_cast<std::uint8_t>(r * 255.0f + 0.5f),
            static_cast<std::uint8_t>(g * 255.0f + 0.5f),
            static_cast<std::uint8_t>(b * 255.0f + 0.5f),
            static_cast<std::uint8_t>(c.alpha * 255.0f)
        };
    }
}