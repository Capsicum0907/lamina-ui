#pragma once

#include <array>
#include <utility>

#include "lamina/shapes/point.hpp"

namespace lamina::shapes {

struct Bounds {
    float left;
    float top;
    float right;
    float bottom;
};

class Cubic {
public:
    Cubic() = default;
    explicit Cubic(const std::array<float, 8>& points) : points_(points) {}
    Cubic(float anchor0_x, float anchor0_y, float control0_x, float control0_y, float control1_x, float control1_y,
          float anchor1_x, float anchor1_y);
    Cubic(Point anchor0, Point control0, Point control1, Point anchor1);

    float anchor0_x() const { return points_[0]; }
    float anchor0_y() const { return points_[1]; }
    float control0_x() const { return points_[2]; }
    float control0_y() const { return points_[3]; }
    float control1_x() const { return points_[4]; }
    float control1_y() const { return points_[5]; }
    float anchor1_x() const { return points_[6]; }
    float anchor1_y() const { return points_[7]; }

    Point anchor0() const { return {points_[0], points_[1]}; }
    Point control0() const { return {points_[2], points_[3]}; }
    Point control1() const { return {points_[4], points_[5]}; }
    Point anchor1() const { return {points_[6], points_[7]}; }

    const std::array<float, 8>& points() const { return points_; }
    std::array<float, 8>& points() { return points_; }

    Point point_on_curve(float t) const;
    bool zero_length() const;
    bool convex_to(const Cubic& next) const;
    Bounds calculate_bounds(bool approximate = false) const;

    std::pair<Cubic, Cubic> split(float t) const;
    Cubic reverse() const;
    Cubic transformed(const PointTransformer& f) const;

    Cubic operator+(const Cubic& o) const;
    Cubic operator*(float x) const;
    Cubic operator/(float x) const;
    bool operator==(const Cubic&) const = default;

    static Cubic straight_line(float x0, float y0, float x1, float y1);
    static Cubic circular_arc(float center_x, float center_y, float x0, float y0, float x1, float y1);
    static Cubic empty(float x0, float y0);
    static Cubic interpolate(const Cubic& c1, const Cubic& c2, float progress);

private:
    std::array<float, 8> points_{};
};

}
