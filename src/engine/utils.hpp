#pragma once

#include <cstdint>
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
        return {
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
}