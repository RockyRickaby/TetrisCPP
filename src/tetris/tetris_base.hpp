#pragma once

#include <cstdint>
#include <iostream>

// Implemented in base.cpp and pieces.cpp
namespace Tetris {
    // already available in SDL as SDL_Color
    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        bool operator==(const Color other) const;
        bool operator!=(const Color other) const;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };
 
    struct Vec2 {
        float x = 0;
        float y = 0;

        Vec2 operator+(const Vec2 other) const;
        Vec2 operator-(const Vec2 other) const;
        Vec2 operator*(float scalar) const;
        Vec2& operator+=(const Vec2 other);
        Vec2& operator-=(const Vec2 other);
        Vec2& operator*=(float scalar);
        bool operator==(const Vec2 other) const;
        bool operator!=(const Vec2 other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

    namespace Tetrimino {
        enum class Rotation {
            NONE,
            CLOCKWISE,
            COUNTERCLOCKWISE,
        };
        
        enum class Type {
            NONE, I, J, L, O, S, Z, T, CUSTOM
        };
    };
}