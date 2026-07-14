#include <iostream>

#include "tetris.hpp"

namespace Tetris {
    Vec2 Vec2::operator+(const Vec2 &other) const {
        float x1 = this->x + other.x;
        float y1 = this->y + other.y;
        return Vec2{ x1, y1 };
    }

    Vec2& Vec2::operator+=(const Vec2 &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2 Vec2::operator-(const Vec2 &other) const {
        float x1 = this->x - other.x;
        float y1 = this->y - other.y;
        return Vec2{ x1, y1 };
    }

    Vec2& Vec2::operator-=(const Vec2 &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    bool Vec2::operator==(const Vec2 &other) const {
        return this->x == other.x && this->y == other.y;
    }

    bool Vec2::operator!=(const Vec2 &other) const {
        return this->x != other.x && this->y != other.y;
    }

    Block Block::operator+(const Vec2 &other) const {
        Block b = *this;
        b.pos += other;
        return b;
    }

    Block Block::operator-(const Vec2 &other) const {
        Block b = *this;
        b.pos -= other;
        return b;
    }

    Block& Block::operator+=(const Vec2 &other) {
        pos += other;
        return *this;
    }

    Block& Block::operator-=(const Vec2 &other) {
        pos -= other;
        return *this;
    }

    std::ostream& operator<<(std::ostream& output, const Vec2& v) {
        output << '(' << v.x << ',' << v.y << ')';
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const Color& v) {
        output << "R:" << v.r << " G:" << v.g << " B:" << v.b << " A:" << v.a;
        return output;
    }
}