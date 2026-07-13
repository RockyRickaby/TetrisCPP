#include <string>
#include <cstdint>
#include <cctype>
#include "tetris.hpp"
#include "tetris_utils.hpp"

// assumes that h is either within the range [0-9] or [A-F]
static inline int char_to_int(char h) {
    return h >= '0' && h <= '9' ? h - '0' : std::toupper(static_cast<unsigned char>(h)) - 'A' + 10;
}

namespace TetrisUtils {
    Tetris::Color from_hex(std::string_view hex, int alpha) {
        // uint32_t color = std::stoi(hex, nullptr, 16);
        if (hex[0] == '#') {
            hex = hex.substr(1);
        }
        return Tetris::Color{
            .r = char_to_int(hex[0]) * 16 + char_to_int(hex[1]),
            .g = char_to_int(hex[2]) * 16 + char_to_int(hex[3]),
            .b = char_to_int(hex[4]) * 16 + char_to_int(hex[5]),
            .a = alpha
        };
    }

    // std::string to_hex(Tetris::Color c) {

    // }
}