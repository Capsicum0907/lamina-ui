#include "lamina/render/polyline.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::render {

Polyline::Polyline(std::vector<Point> points) : points_(std::move(points)) {
    lengths_.reserve(points_.size());
    double total = 0.0;
    for (std::size_t i = 0; i < points_.size(); ++i) {
        if (i > 0) {
            const Point d = points_[i] - points_[i - 1];
            total += std::hypot(d.x, d.y);
        }
        lengths_.push_back(total);
    }
}

Point Polyline::at(double distance) const {
    if (points_.empty()) {
        return {0.0, 0.0};
    }
    if (distance <= 0.0) {
        return points_.front();
    }
    if (distance >= length()) {
        return points_.back();
    }
    const auto it = std::upper_bound(lengths_.begin(), lengths_.end(), distance);
    const std::size_t i = static_cast<std::size_t>(it - lengths_.begin());
    const double span = lengths_[i] - lengths_[i - 1];
    const double k = span > 0.0 ? (distance - lengths_[i - 1]) / span : 0.0;
    return points_[i - 1] + (points_[i] - points_[i - 1]) * k;
}

std::vector<Point> Polyline::segment(double from, double to) const {
    std::vector<Point> out;
    if (points_.size() < 2) {
        return out;
    }
    from = std::clamp(from, 0.0, length());
    to = std::clamp(to, 0.0, length());
    if (to <= from) {
        return out;
    }
    out.push_back(at(from));
    const auto first = std::upper_bound(lengths_.begin(), lengths_.end(), from);
    for (auto it = first; it != lengths_.end() && *it < to; ++it) {
        out.push_back(points_[static_cast<std::size_t>(it - lengths_.begin())]);
    }
    out.push_back(at(to));
    return out;
}

void append_cubic(std::vector<Point>& out, Point p0, Point c0, Point c1, Point p1, int segments) {
    if (out.empty()) {
        out.push_back(p0);
    }
    for (int i = 1; i <= segments; ++i) {
        const double t = static_cast<double>(i) / segments;
        const double u = 1.0 - t;
        out.push_back(p0 * (u * u * u) + c0 * (3.0 * u * u * t) + c1 * (3.0 * u * t * t) + p1 * (t * t * t));
    }
}

void append_quadratic(std::vector<Point>& out, Point p0, Point c, Point p1, int segments) {
    if (out.empty()) {
        out.push_back(p0);
    }
    for (int i = 1; i <= segments; ++i) {
        const double t = static_cast<double>(i) / segments;
        const double u = 1.0 - t;
        out.push_back(p0 * (u * u) + c * (2.0 * u * t) + p1 * (t * t));
    }
}

}
