#pragma once

#include <vector>
#include <SDL3/SDL.h>

#include "hermite.hpp"
#include "splines.hpp"
#include "../tengine.hpp"

namespace TEngine::Shape {
    struct Triangle {
        Vec2 p1, p2, p3;
        Color color = {};
    };

    struct Circle {
        Vec2 pos;
        float radius;
    };

    struct Rectangle {
        Vec2 pos;
        Vec2 size;
    };

    void draw_circle(SDL_Renderer* renderer, float x, float y, float r);
    void fill_circle(SDL_Renderer* renderer, float x, float y, float r);
    inline void draw_circle(SDL_Renderer* renderer, Circle c) { draw_circle(renderer, c.pos.x, c.pos.y, c.radius); }
    inline void fill_circle(SDL_Renderer* renderer, Circle c) { fill_circle(renderer, c.pos.x, c.pos.y, c.radius); }

    void draw_rect(SDL_Renderer* renderer, float x, float y, float w, float h);
    void fill_rect(SDL_Renderer* renderer, float x, float y, float w, float h);
    inline void draw_rect(SDL_Renderer* renderer, Rectangle rect) { draw_rect(renderer, rect.pos.x, rect.pos.y, rect.size.x, rect.size.y); }
    inline void fill_rect(SDL_Renderer* renderer, Rectangle rect) { fill_rect(renderer, rect.pos.x, rect.pos.y, rect.size.x, rect.size.y); }

    void draw_tri(SDL_Renderer* renderer, Triangle tri);
    void fill_tri(SDL_Renderer* renderer, Triangle tri);

    void draw_bezier2d_quad(SDL_Renderer* renderer, Splines::Bezier2D::QuadraticBezier curve, std::uint32_t points);
    void draw_bezier2d_cubic(SDL_Renderer* renderer, Splines::Bezier2D::CubicBezier curve, std::uint32_t points);
    void draw_hermite2d(SDL_Renderer* renderer, Splines::Hermite2D::HermiteCurve curve, std::uint32_t points);

    template<Splines::Spline2DConcept T>
    void draw_spline2d(SDL_Renderer* renderer, T curve, std::uint32_t points) {
        float inc = 1.0f / static_cast<float>(points + 1);
        float t = inc;

        const Vec2 begin = Splines::spline2d(curve, 0);
        const Vec2 end = Splines::spline2d(curve, 1);

        std::vector<SDL_FPoint> lines;
        lines.reserve(2 + points);
        lines.emplace_back(begin.x, begin.y);
        while (t < 1.0f) {
            Vec2 vec = Splines::spline2d(curve, t);
            lines.emplace_back(vec.x, vec.y);
            t += inc;
        }
        lines.emplace_back(end.x, end.y);
        SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
    }
};