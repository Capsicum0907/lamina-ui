#pragma once

#include <algorithm>
#include <cmath>

namespace lamina::motion {

struct CubicBezier {
    double x1;
    double y1;
    double x2;
    double y2;

    bool operator==(const CubicBezier&) const = default;
};

inline constexpr int ease_solve_iterations = 48;
inline constexpr double ease_solve_tolerance = 1e-7;

constexpr double bezier_coordinate(double p1, double p2, double t) {
    const double u = 1.0 - t;
    return 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t;
}

inline double ease(const CubicBezier& curve, double fraction) {
    const double x = std::clamp(fraction, 0.0, 1.0);
    if (x <= 0.0 || x >= 1.0) {
        return x;
    }
    double lo = 0.0;
    double hi = 1.0;
    double t = x;
    for (int i = 0; i < ease_solve_iterations; ++i) {
        const double at = bezier_coordinate(curve.x1, curve.x2, t);
        if (std::abs(at - x) < ease_solve_tolerance) {
            break;
        }
        if (at < x) {
            lo = t;
        } else {
            hi = t;
        }
        t = (lo + hi) / 2.0;
    }
    return bezier_coordinate(curve.y1, curve.y2, t);
}

inline double ease_inverse(const CubicBezier& curve, double eased) {
    const double y = std::clamp(eased, 0.0, 1.0);
    if (y <= 0.0 || y >= 1.0) {
        return y;
    }
    double lo = 0.0;
    double hi = 1.0;
    double x = y;
    for (int i = 0; i < ease_solve_iterations; ++i) {
        const double at = ease(curve, x);
        if (std::abs(at - y) < ease_solve_tolerance) {
            break;
        }
        if (at < y) {
            lo = x;
        } else {
            hi = x;
        }
        x = (lo + hi) / 2.0;
    }
    return x;
}

}
