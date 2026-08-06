#include <iostream>
#include "tetris_base.hpp"

namespace Tetris {

    bool Color::operator==(const Color other) const {
        return
            r == other.r &&
            g == other.g &&
            b == other.b &&
            a == other.a;
    }

    bool Color::operator!=(const Color other) const {
        return
            r != other.r ||
            g != other.g ||
            b != other.b ||
            a != other.a;   
    }

    
    Vec2 Vec2::operator+(const Vec2 other) const {
        float x1 = this->x + other.x;
        float y1 = this->y + other.y;
        return Vec2{x1, y1};
    }

    Vec2 Vec2::operator*(float scalar) const {
        float x1 = this->x * scalar;
        float y1 = this->y * scalar;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator+=(const Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2 Vec2::operator-(const Vec2 other) const {
        float x1 = this->x - other.x;
        float y1 = this->y - other.y;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator-=(const Vec2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& Vec2::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    bool Vec2::operator==(const Vec2 other) const {
        return this->x == other.x && this->y == other.y;
    }

    bool Vec2::operator!=(const Vec2 other) const {
        return this->x != other.x || this->y != other.y;
    }

    std::ostream& operator<<(std::ostream& output, const Vec2& v) {
        output << '(' << v.x << ',' << v.y << ')';
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const Color& v) {
        output << "R:" << static_cast<int>(v.r)
               << " G:" << static_cast<int>(v.g)
               << " B:" << static_cast<int>(v.b)
               << " A:" << static_cast<int>(v.a);
        return output;
    }
}
