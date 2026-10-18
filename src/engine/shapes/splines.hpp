#pragma once

#include "bezier.hpp"
#include "hermite.hpp"

namespace TEngine::Splines {
    constexpr auto hermite_to_bezier(const Hermite2D::HermiteCurve& curve) -> Bezier2D::CubicBezier {
        return {
            .p0 = {curve.p0},
            .p1 = {curve.p0 + (curve.v0 * (1.0f / 3.0f))},
            .p2 = {curve.p1 - (curve.v1 * (1.0f / 3.0f))},
            .p3 = {curve.p1},
        };
    }

    constexpr auto bezier_to_hermite(const Bezier2D::CubicBezier& curve) -> Hermite2D::HermiteCurve {
        return {
            .p0 = {curve.p0},
            .p1 = {curve.p3},
            .v0 = {3 * (curve.p1 - curve.p0)},
            .v1 = {3 * (curve.p3 - curve.p2)}
        };
    }

    template<typename T>
    concept Spline2DConcept =
        std::is_same_v<T, Bezier2D::QuadraticBezier> ||
        std::is_same_v<T, Bezier2D::CubicBezier> ||
        std::is_same_v<T, Hermite2D::HermiteCurve>;
    
    template<typename T>
    concept NotASpline = false;

    template<Spline2DConcept Curve>
    constexpr auto spline2d(const Curve& curve, float t) -> Vec2 {
        if constexpr (std::is_same_v<Curve, Bezier2D::QuadraticBezier>) {
            return Bezier2D::quadratic_bezier(curve.p0, curve.p1, curve.p2, t);
        } else if constexpr (std::is_same_v<Curve, Bezier2D::CubicBezier>) {
            return Bezier2D::cubic_bezier(curve.p0, curve.p1, curve.p2, curve.p3, t);
        } else if constexpr (std::is_same_v<Curve, Hermite2D::HermiteCurve>) {
            return Hermite2D::hermite(curve.p0, curve.p1, curve.v0, curve.v1, t);
        }
    }
}