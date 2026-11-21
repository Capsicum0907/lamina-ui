#pragma once

#include <span>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::render {

class Polyline {
public:
    Polyline() = default;
    explicit Polyline(std::vector<Point> points);

    const std::vector<Point>& points() const { return points_; }
    double length() const { return lengths_.empty() ? 0.0 : lengths_.back(); }
    Point at(double distance) const;
    std::vector<Point> segment(double from, double to) const;

private:
    std::vector<Point> points_;
    std::vector<double> lengths_;
};

void append_cubic(std::vector<Point>& out, Point p0, Point c0, Point c1, Point p1, int segments);
void append_quadratic(std::vector<Point>& out, Point p0, Point c, Point p1, int segments);

}
