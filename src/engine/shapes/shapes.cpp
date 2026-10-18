#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <print>

#include "shapes.hpp"
#include "bezier.hpp"

namespace TEngine::Shape {
    void draw_circle(SDL_Renderer* renderer, float x, float y, float r) {
        float c_x = r;
        float c_y = 0;
        float p = 1 - r;

        static constexpr std::array quads = {
            Vec2{1, 1},
            Vec2{1, -1},
            Vec2{-1, 1},
            Vec2{-1, -1},
        };

        std::array<SDL_FPoint, 8> points;

        while (c_x >= c_y) {
            int i = 0;
            for (const auto [j, k] : quads) {
                points[i] = {j * c_x + x, k * c_y + y};
                points[i + 4] = {k * c_y + x, j * c_x + y};
                i++;
            }
            SDL_RenderPoints(renderer, points.data(), static_cast<int>(points.size()));
            c_y = c_y + 1;
            c_x = c_x - (p > 0 ? 1 : 0);
            p += (p > 0 ? (1 - 2 * c_x + 2 * c_y) : (1 + 2 * c_y));
        }
    }

    [[maybe_unused]]
    static void fill_circle_more_mem_fixed(SDL_Renderer* renderer, float x, float y, float r) {
        float c_x = r;
        float c_y = 0;
        float p = 1.25 - r;

        static constexpr std::array quads = {
            Vec2{1, 1},
            Vec2{-1, 1},
            Vec2{1, -1},
            Vec2{-1, -1},
        };

        std::vector<SDL_FPoint> lines;
        lines.reserve(std::ceil(5.701 * r));

        while (c_x >= c_y) {
            int i = 0;
            std::array<SDL_FPoint, 8> lines_arr;
            for (const auto [j, k] : quads) {
                lines_arr.at(i) = {c_x * j + x, c_y * k + y};
                lines_arr.at(i + 4) = {c_y * k + x, c_x * j + y};
                i++;
            }
            lines.insert_range(lines.begin() + lines.size(), lines_arr);
            c_y = c_y + 1;
            c_x = c_x - (p > 0 ? 1 : 0);
            p += (p > 0 ? (1 - 2 * c_x + 2 * c_y) : (1 + 2 * c_y));
        }
        SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
    }

    [[maybe_unused]]
    static void fill_circle_less_mem_fixed(SDL_Renderer* renderer, float x, float y, float r) {
        float c_x = r;
        float c_y = 0;
        float p = 1 - r;

        static constexpr std::array quads = {
            Vec2{1, 1},
            Vec2{-1, 1},
            Vec2{1, -1},
            Vec2{-1, -1},
        };

        std::array<SDL_FPoint, 8> lines;

        while (c_x >= c_y) {
            int i = 0;
            for (const auto [j, k] : quads) {
                lines.at(i) = {c_x * j + x, c_y * k + y};
                lines.at(i + 4) = {c_y * k + x, c_x * j + y};
                i++;
            }
            SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
            c_y = c_y + 1;
            c_x = c_x - (p > 0 ? 1 : 0);
            p += (p > 0 ? (1 - 2 * c_x + 2 * c_y) : (1 + 2 * c_y));
        }
    }

    void fill_circle(SDL_Renderer *renderer, float x, float y, float r) {
        fill_circle_less_mem_fixed(renderer, x, y, r);
    }
    
    void draw_rect(SDL_Renderer* renderer, float x, float y, float w, float h) {
        SDL_FRect r{x, y, w, h};
        SDL_RenderRect(renderer, &r);
    }

    void fill_rect(SDL_Renderer* renderer, float x, float y, float w, float h) {
        SDL_FRect r{x, y, w, h};
        SDL_RenderFillRect(renderer, &r);
    }

    void draw_tri(SDL_Renderer* renderer, Triangle tri) {
        std::array points = {
            SDL_FPoint{tri.p1.x, tri.p1.y},
            SDL_FPoint{tri.p2.x, tri.p2.y},
            SDL_FPoint{tri.p3.x, tri.p3.y},
            SDL_FPoint{tri.p1.x, tri.p1.y},
        };
        SDL_RenderLines(renderer, points.data(), static_cast<int>(points.size()));
    }

    void fill_tri(SDL_Renderer* renderer, Triangle tri) {
        SDL_FColor color{tri.color.r / 255.f, tri.color.g / 255.f, tri.color.b / 255.f, tri.color.a / 255.f};
        std::array<SDL_Vertex, 3> vertices;
        vertices[0] = {
            .position = {tri.p1.x, tri.p1.y},
            .color = color,
            .tex_coord = {0, 0}
        };

        vertices[1] = {
            .position = {tri.p2.x, tri.p2.y},
            .color = color,
            .tex_coord = {0, 0}
        };

        vertices[2] = {
            .position = {tri.p3.x, tri.p3.y},
            .color = color,
            .tex_coord = {0, 0}
        };

        SDL_RenderGeometry(renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), nullptr, 0);
    }

    void draw_bezier2d_quad(SDL_Renderer* renderer, Splines::Bezier2D::QuadraticBezier curve, std::uint32_t points) {
        float inc = 1.0f / static_cast<float>(points);
        float t = inc;

        const Vec2 begin = Splines::Bezier2D::quadratic_bezier(curve.p0, curve.p1, curve.p2, 0);
        const Vec2 end = Splines::Bezier2D::quadratic_bezier(curve.p0, curve.p1, curve.p2, 1);

        std::vector<SDL_FPoint> lines;
        lines.reserve(2 + (points - 1));
        lines.emplace_back(begin.x, begin.y);
        while (t < 1.0f) {
            Vec2 vec = Splines::Bezier2D::quadratic_bezier(curve.p0, curve.p1, curve.p2, t);
            lines.emplace_back(vec.x, vec.y);
            t += inc;
        }
        lines.emplace_back(end.x, end.y);
        SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
    }
    
    void draw_bezier2d_cubic(SDL_Renderer* renderer, Splines::Bezier2D::CubicBezier curve, std::uint32_t points) {
        float inc = 1.0f / static_cast<float>(points);
        float t = inc;

        const Vec2 begin = Splines::Bezier2D::cubic_bezier(curve.p0, curve.p1, curve.p2, curve.p3, 0);
        const Vec2 end = Splines::Bezier2D::cubic_bezier(curve.p0, curve.p1, curve.p2, curve.p3, 1);

        std::vector<SDL_FPoint> lines;
        lines.reserve(2 + (points - 1));
        lines.emplace_back(begin.x, begin.y);
        while (t < 1.0f) {
            Vec2 vec = Splines::Bezier2D::cubic_bezier(curve.p0, curve.p1, curve.p2, curve.p3, t);
            lines.emplace_back(vec.x, vec.y);
            t += inc;
        }
        lines.emplace_back(end.x, end.y);
        SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
    }

    void draw_hermite2d(SDL_Renderer* renderer, Splines::Hermite2D::HermiteCurve curve, std::uint32_t points) {
        float inc = 1.0f / static_cast<float>(points);
        float t = inc;

        const Vec2 begin = Splines::Hermite2D::hermite(curve.p0, curve.p1, curve.v0, curve.v1, 0);
        const Vec2 end = Splines::Hermite2D::hermite(curve.p0, curve.p1, curve.v0, curve.v1, 1);

        std::vector<SDL_FPoint> lines;
        lines.reserve(2 + (points - 1));
        lines.emplace_back(begin.x, begin.y);
        while (t < 1.0f) {
            Vec2 vec = Splines::Hermite2D::hermite(curve.p0, curve.p1, curve.v0, curve.v1, t);
            lines.emplace_back(vec.x, vec.y);
            t += inc;
        }
        lines.emplace_back(end.x, end.y);
        SDL_RenderLines(renderer, lines.data(), static_cast<int>(lines.size()));
    }
};