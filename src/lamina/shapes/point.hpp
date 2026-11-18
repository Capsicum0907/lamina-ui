#pragma once

#include <cmath>
#include <functional>
#include <numbers>

namespace lamina::shapes {

struct Point {
    float x = 0.0f;
    float y = 0.0f;

    constexpr bool operator==(const Point&) const = default;
};

using PointTransformer = std::function<Point(Point)>;

inline constexpr float distance_epsilon = 1e-4f;
inline constexpr float angle_epsilon = 1e-6f;
inline constexpr float float_pi = std::numbers::pi_v<float>;
inline constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;

inline float jvm_cos(float x) { return static_cast<float>(std::cos(static_cast<double>(x))); }
inline float jvm_sin(float x) { return static_cast<float>(std::sin(static_cast<double>(x))); }
inline float jvm_atan2(float y, float x) {
    return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
}

constexpr Point operator-(Point p) { return {-p.x, -p.y}; }
constexpr Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
constexpr Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
constexpr Point operator*(Point p, float k) { return {p.x * k, p.y * k}; }
constexpr Point operator/(Point p, float k) { return {p.x / k, p.y / k}; }

inline float get_distance(Point p) { return std::sqrt(p.x * p.x + p.y * p.y); }
constexpr float get_distance_squared(Point p) { return p.x * p.x + p.y * p.y; }
constexpr float dot_product(Point a, Point b) { return a.x * b.x + a.y * b.y; }
constexpr bool clockwise(Point a, Point b) { return a.x * b.y - a.y * b.x > 0.0f; }
constexpr Point rotate90(Point p) { return {-p.y, p.x}; }

Point get_direction(Point p);

inline float distance(float x, float y) { return std::sqrt(x * x + y * y); }
constexpr float distance_squared(float x, float y) { return x * x + y * y; }
constexpr float square(float x) { return x * x; }

Point direction_vector(float x, float y);
inline Point direction_vector(float angle_radians) { return {jvm_cos(angle_radians), jvm_sin(angle_radians)}; }
inline Point radial_to_cartesian(float radius, float angle_radians, Point center = {}) {
    return direction_vector(angle_radians) * radius + center;
}

constexpr float interpolate(float start, float stop, float fraction) {
    return (1.0f - fraction) * start + fraction * stop;
}
constexpr Point interpolate(Point start, Point stop, float fraction) {
    return {interpolate(start.x, stop.x, fraction), interpolate(start.y, stop.y, fraction)};
}

inline float positive_modulo(float num, float mod) { return std::fmod(std::fmod(num, mod) + mod, mod); }

bool convex(Point previous, Point current, Point next);

}
