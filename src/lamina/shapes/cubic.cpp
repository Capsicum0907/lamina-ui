#include "lamina/shapes/cubic.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::shapes {

namespace {

constexpr float circular_arc_straight_cos = 0.999f;
constexpr float one_third = 1.0f / 3.0f;
constexpr float two_thirds = 2.0f / 3.0f;

bool zero_ish(float value) {
    return std::abs(value) < distance_epsilon;
}

bool in_unit(float t) {
    return t >= 0.0f && t <= 1.0f;
}

}

Cubic::Cubic(float anchor0_x, float anchor0_y, float control0_x, float control0_y, float control1_x, float control1_y,
             float anchor1_x, float anchor1_y)
    : points_{anchor0_x, anchor0_y, control0_x, control0_y, control1_x, control1_y, anchor1_x, anchor1_y} {}

Cubic::Cubic(Point anchor0, Point control0, Point control1, Point anchor1)
    : points_{anchor0.x, anchor0.y, control0.x, control0.y, control1.x, control1.y, anchor1.x, anchor1.y} {}

Point Cubic::point_on_curve(float t) const {
    const float u = 1.0f - t;
    return {
        anchor0_x() * (u * u * u) + control0_x() * (3.0f * t * u * u) + control1_x() * (3.0f * t * t * u) +
            anchor1_x() * (t * t * t),
        anchor0_y() * (u * u * u) + control0_y() * (3.0f * t * u * u) + control1_y() * (3.0f * t * t * u) +
            anchor1_y() * (t * t * t),
    };
}

bool Cubic::zero_length() const {
    return std::abs(anchor0_x() - anchor1_x()) < distance_epsilon &&
           std::abs(anchor0_y() - anchor1_y()) < distance_epsilon;
}

bool Cubic::convex_to(const Cubic& next) const {
    return convex(anchor0(), anchor1(), next.anchor1());
}

Bounds Cubic::calculate_bounds(bool approximate) const {
    if (zero_length()) {
        return {anchor0_x(), anchor0_y(), anchor0_x(), anchor0_y()};
    }

    float min_x = std::min(anchor0_x(), anchor1_x());
    float min_y = std::min(anchor0_y(), anchor1_y());
    float max_x = std::max(anchor0_x(), anchor1_x());
    float max_y = std::max(anchor0_y(), anchor1_y());

    if (approximate) {
        return {
            std::min(min_x, std::min(control0_x(), control1_x())),
            std::min(min_y, std::min(control0_y(), control1_y())),
            std::max(max_x, std::max(control0_x(), control1_x())),
            std::max(max_y, std::max(control0_y(), control1_y())),
        };
    }

    auto extend = [this](float a, float b, float c, bool use_x, float& lo, float& hi) {
        auto consider = [&](float t) {
            if (in_unit(t)) {
                const Point p = point_on_curve(t);
                const float v = use_x ? p.x : p.y;
                if (v < lo) {
                    lo = v;
                }
                if (v > hi) {
                    hi = v;
                }
            }
        };
        if (zero_ish(a)) {
            if (b != 0.0f) {
                consider(2.0f * c / (-2.0f * b));
            }
        } else {
            const float s = b * b - 4.0f * a * c;
            if (s >= 0.0f) {
                consider((-b + std::sqrt(s)) / (2.0f * a));
                consider((-b - std::sqrt(s)) / (2.0f * a));
            }
        }
    };

    extend(-anchor0_x() + 3.0f * control0_x() - 3.0f * control1_x() + anchor1_x(),
           2.0f * anchor0_x() - 4.0f * control0_x() + 2.0f * control1_x(), -anchor0_x() + control0_x(), true, min_x,
           max_x);
    extend(-anchor0_y() + 3.0f * control0_y() - 3.0f * control1_y() + anchor1_y(),
           2.0f * anchor0_y() - 4.0f * control0_y() + 2.0f * control1_y(), -anchor0_y() + control0_y(), false, min_y,
           max_y);
    return {min_x, min_y, max_x, max_y};
}

std::pair<Cubic, Cubic> Cubic::split(float t) const {
    const float u = 1.0f - t;
    const Point p = point_on_curve(t);
    return {
        Cubic(anchor0_x(), anchor0_y(), anchor0_x() * u + control0_x() * t, anchor0_y() * u + control0_y() * t,
              anchor0_x() * (u * u) + control0_x() * (2.0f * u * t) + control1_x() * (t * t),
              anchor0_y() * (u * u) + control0_y() * (2.0f * u * t) + control1_y() * (t * t), p.x, p.y),
        Cubic(p.x, p.y, control0_x() * (u * u) + control1_x() * (2.0f * u * t) + anchor1_x() * (t * t),
              control0_y() * (u * u) + control1_y() * (2.0f * u * t) + anchor1_y() * (t * t),
              control1_x() * u + anchor1_x() * t, control1_y() * u + anchor1_y() * t, anchor1_x(), anchor1_y()),
    };
}

Cubic Cubic::reverse() const {
    return Cubic(anchor1_x(), anchor1_y(), control1_x(), control1_y(), control0_x(), control0_y(), anchor0_x(),
                 anchor0_y());
}

Cubic Cubic::transformed(const PointTransformer& f) const {
    return Cubic(f(anchor0()), f(control0()), f(control1()), f(anchor1()));
}

Cubic Cubic::operator+(const Cubic& o) const {
    std::array<float, 8> r{};
    for (std::size_t i = 0; i < r.size(); ++i) {
        r[i] = points_[i] + o.points_[i];
    }
    return Cubic(r);
}

Cubic Cubic::operator*(float x) const {
    std::array<float, 8> r{};
    for (std::size_t i = 0; i < r.size(); ++i) {
        r[i] = points_[i] * x;
    }
    return Cubic(r);
}

Cubic Cubic::operator/(float x) const {
    return *this * (1.0f / x);
}

Cubic Cubic::straight_line(float x0, float y0, float x1, float y1) {
    return Cubic(x0, y0, shapes::interpolate(x0, x1, one_third), shapes::interpolate(y0, y1, one_third),
                 shapes::interpolate(x0, x1, two_thirds), shapes::interpolate(y0, y1, two_thirds), x1, y1);
}

Cubic Cubic::circular_arc(float center_x, float center_y, float x0, float y0, float x1, float y1) {
    const Point p0d = direction_vector(x0 - center_x, y0 - center_y);
    const Point p1d = direction_vector(x1 - center_x, y1 - center_y);
    const Point rotated_p0 = rotate90(p0d);
    const Point rotated_p1 = rotate90(p1d);
    const bool is_clockwise = dot_product(rotated_p0, Point{x1 - center_x, y1 - center_y}) >= 0.0f;
    const float cosa = dot_product(p0d, p1d);
    if (cosa > circular_arc_straight_cos) {
        return straight_line(x0, y0, x1, y1);
    }
    const float k = distance(x0 - center_x, y0 - center_y) * 4.0f / 3.0f *
                    (std::sqrt(2.0f * (1.0f - cosa)) - std::sqrt(1.0f - cosa * cosa)) / (1.0f - cosa) *
                    (is_clockwise ? 1.0f : -1.0f);
    return Cubic(x0, y0, x0 + rotated_p0.x * k, y0 + rotated_p0.y * k, x1 - rotated_p1.x * k, y1 - rotated_p1.y * k,
                 x1, y1);
}

Cubic Cubic::empty(float x0, float y0) {
    return Cubic(x0, y0, x0, y0, x0, y0, x0, y0);
}

Cubic Cubic::interpolate(const Cubic& c1, const Cubic& c2, float progress) {
    std::array<float, 8> r{};
    for (std::size_t i = 0; i < r.size(); ++i) {
        r[i] = shapes::interpolate(c1.points_[i], c2.points_[i], progress);
    }
    return Cubic(r);
}

}
