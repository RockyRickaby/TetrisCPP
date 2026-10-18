#pragma once

#include "../tengine.hpp"

namespace TEngine::Splines::Hermite2D {
    struct HermiteCurve {
        Vec2 p0; // starting point
        Vec2 p1; // ending point
        Vec2 v0; // tangent vector
        Vec2 v1; // tangent vector
    };

    constexpr Vec2 hermite(Vec2 p0, Vec2 p1, Vec2 v0, Vec2 v1, float t) {
        const float t_square = t * t;
        const float t_cubed = t * t * t;
        
        Vec2 res = (2 * t_cubed - 3 * t_square + 1) * p0 +
            (t_cubed - 2 * t_square + t) * v0 +
            (-2 * t_cubed + 3 * t_square) * p1 +
            (t_cubed - t_square) * v1;

        return res;
    }

    constexpr Vec2 hermite(const HermiteCurve& curve, float t) {
        return hermite(curve.p0, curve.p1, curve.v0, curve.v1, t);
    }
}

template<>
struct std::formatter<TEngine::Splines::Hermite2D::HermiteCurve> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("HermiteCurve formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }
    auto format(const TEngine::Splines::Hermite2D::HermiteCurve& c, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "P0={} P1={} M0={} M1={}", c.p0, c.p1, c.v0, c.v1);
    }
};