#pragma once

#include <span>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::render {

enum class PathVerb { move, line, cubic, close };

class Path {
public:
    void move_to(Point p);
    void line_to(Point p);
    void cubic_to(Point c1, Point c2, Point end);
    void close();
    void append(const Path& other);

    const std::vector<PathVerb>& verbs() const { return verbs_; }
    const std::vector<Point>& points() const { return points_; }
    bool empty() const { return verbs_.empty(); }

    Path transformed(const Affine& m) const;

private:
    std::vector<PathVerb> verbs_;
    std::vector<Point> points_;
};

Path rounded_rect_path(const Rect& rect, const CornerRadii& radii);
Path ring_path(const Rect& rect, double radius, double width);
Path circle_path(Point center, double radius);
Path stroke_path(std::span<const Point> centerline, double width);

}
