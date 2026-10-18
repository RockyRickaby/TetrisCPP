#pragma once

#include "../tengine.hpp"
#include <format>

namespace TEngine::Splines::Bezier2D {
    struct QuadraticBezier {
        Vec2 p0{}; // starting point
        Vec2 p1{}; // control point c1
        Vec2 p2{}; // ending point
    };

    struct CubicBezier {
        Vec2 p0{}; // starting point
        Vec2 p1{}; // control point c1
        Vec2 p2{}; // control point c2
        Vec2 p3{}; // ending point
    };

    constexpr Vec2 quadratic_bezier(Vec2 p0, Vec2 p1, Vec2 p2, float t) {
        const float min_t = 1 - t;
        Vec2 res = min_t * min_t * p0 
            + 2 * min_t * t * p1
            + t * t * p2;
        return res;
    }

    constexpr Vec2 cubic_bezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
        const float min_t = 1 - t;
        Vec2 res = min_t * min_t * min_t * p0
            + 3 * min_t * min_t * t * p1
            + 3 * min_t * t * t * p2
            + t * t * t * p3;
        return res;
    }

    template<typename T>
    concept BezierCurve = std::is_same_v<T, QuadraticBezier> || std::is_same_v<T, CubicBezier>;

    template<typename T>
    concept NotBezier = false;

    template<BezierCurve Curve>
    constexpr auto bezier(const Curve& curve, float t) {
        if constexpr (std::is_same_v<Curve, QuadraticBezier>) {
            return quadratic_bezier(curve.p0, curve.p1, curve.p2, t);
        } else if constexpr (std::is_same_v<Curve, CubicBezier>) {
            return cubic_bezier(curve.p0, curve.p1, curve.p2, curve.p3, t);
        } else {
            static_assert(NotBezier<Curve>, "type specified is not a bezier curve");
        }
    }
}

template<>
struct std::formatter<TEngine::Splines::Bezier2D::QuadraticBezier> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("QuadraticBezier formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }
    auto format(const TEngine::Splines::Bezier2D::QuadraticBezier& b, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {}", b.p0, b.p1, b.p2);
    }
};

template<>
struct std::formatter<TEngine::Splines::Bezier2D::CubicBezier> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("CubicBezier formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }
    auto format(const TEngine::Splines::Bezier2D::CubicBezier& b, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {} {}", b.p0, b.p1, b.p2, b.p3);
    }
};