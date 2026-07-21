#pragma once

#include <string>
#include <SDL3/SDL_stdinc.h>
#include "tetris_base.hpp"

namespace TetrisUtils {
    namespace __Eval {
        inline constexpr unsigned char to_uppercase(unsigned char h) {
            return (h & ~(1 << 5));
        }

        // if h is a letter, it will be converted to its upper case version
        // assumes that h is either within the range [0-9] or [a-fA-F]
        // behavior not defined if otherwise
        inline constexpr Uint8 char_to_int(char h) {
            return h >= '0' && h <= '9' ? h - '0' : to_uppercase(static_cast<unsigned char>(h)) - 'A' + 10;
        }
    }
    // assumes the format #RRGGBB. alpha is set to 255 by default
    constexpr Tetris::Color color_from_hex(std::string_view hex, Uint8 alpha = 255U) {
        // uint32_t color = std::stoi(hex, nullptr, 16);
        if (hex[0] == '#') {
            hex = hex.substr(1);
        }
        return Tetris::Color{
            .r = static_cast<Uint8>(__Eval::char_to_int(hex[0]) * 16 + __Eval::char_to_int(hex[1])),
            .g = static_cast<Uint8>(__Eval::char_to_int(hex[2]) * 16 + __Eval::char_to_int(hex[3])),
            .b = static_cast<Uint8>(__Eval::char_to_int(hex[4]) * 16 + __Eval::char_to_int(hex[5])),
            .a = alpha
        };
    }

    // assumes rot has size >=2 and only contains the characters 0, R, 2, L and, optionally, - and > (to form a little arrow)
    // results are undefined if any of the characters are not those listed here
    // if rot represents an invalid rotation, the returned value will also be invalid
    constexpr int rotstr_to_int(std::string_view rot) {
        int fst = 0;
        int lst = rot.length() > 2 ? rot.length() - 1 : 1;
        int res = 0;

        char fst_c = rot[fst], lst_c = rot[lst];
        if (fst_c == 'R') {
            res = 1;
        } else if (fst_c == 'L') {
            res = 3;
        } else {
            res = fst_c - '0';
        }
        res *= 10;

        if (lst_c == 'R') {
            res += 1;
        } else if (lst_c == 'L') {
            res += 3;
        } else {
            res += lst_c - '0';
        }
        return res;
    }
}