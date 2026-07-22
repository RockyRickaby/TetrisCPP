#include <iostream>

#include "tetris_base.hpp"

namespace Tetris {
    namespace Bag {
        Tetrimino::Piece Standard::operator()() {
            // TODO - IMPLEMENT THIS
            return Tetrimino::get_piece(Tetrimino::Type::J);
        }

        void Standard::draw(SDL_Renderer *renderer, SDL_Texture *texture) {
            // TODO - IMPLEMENT THIS
        }

        Tetris::Tetrimino::Piece Random::operator()() {
            // TODO - IMPLEMENT THIS
            return Tetris::Tetrimino::get_piece(Tetris::Tetrimino::Type::J);
        }
        
        void Random::draw(SDL_Renderer *renderer, SDL_Texture *texture) {
            // TODO - IMPLEMENT THIS
        }
    };
    
    Vec2 Vec2::operator+(const Vec2 &other) const {
        float x1 = this->x + other.x;
        float y1 = this->y + other.y;
        return Vec2{x1, y1};
    }

    Vec2 Vec2::operator*(float scalar) const {
        float x1 = this->x * scalar;
        float y1 = this->y * scalar;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator+=(const Vec2 &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2 Vec2::operator-(const Vec2 &other) const {
        float x1 = this->x - other.x;
        float y1 = this->y - other.y;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator-=(const Vec2 &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& Vec2::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    bool Vec2::operator==(const Vec2 &other) const {
        return this->x == other.x && this->y == other.y;
    }

    bool Vec2::operator!=(const Vec2 &other) const {
        return this->x != other.x || this->y != other.y;
    }

    // Block Block::operator+(const Vec2 &other) const {
    //     Block b = *this;
    //     b.pos += other;
    //     return b;
    // }

    // Block Block::operator-(const Vec2 &other) const {
    //     Block b = *this;
    //     b.pos -= other;
    //     return b;
    // }

    // Block& Block::operator+=(const Vec2 &other) {
    //     pos += other;
    //     return *this;
    // }

    // Block& Block::operator-=(const Vec2 &other) {
    //     pos -= other;
    //     return *this;
    // }

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