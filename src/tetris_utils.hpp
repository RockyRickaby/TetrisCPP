#pragma once

#include <string>
#include "tetris.hpp"

namespace TetrisUtils {
    // assumes the format #RRGGBB. alpha is set to 255 by default
    Tetris::Color from_hex(std::string_view hex, int alpha = 255);
    // std::string to_hex(Tetris::Color c);
}