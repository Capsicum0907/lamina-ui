#include "lamina/render/path.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace lamina::render {

void Path::move_to(Point p) {
    verbs_.push_back(PathVerb::move);
    points_.push_back(p);
}

void Path::line_to(Point p) {
    verbs_.push_back(PathVerb::line);
    points_.push_back(p);
}

void Path::cubic_to(Point c1, Point c2, Point end) {
    verbs_.push_back(PathVerb::cubic);
    points_.push_back(c1);
    points_.push_back(c2);
    points_.push_back(end);
}

void Path::close() {
    verbs_.push_back(PathVerb::close);
}

void Path::append(const Path& other) {
    verbs_.insert(verbs_.end(), other.verbs_.begin(), other.verbs_.end());
    points_.insert(points_.end(), other.points_.begin(), other.points_.end());
}

Path Path::transformed(const Affine& m) const {
    Path out;
    out.verbs_ = verbs_;
    out.points_.reserve(points_.size());
    for (const Point& p : points_) {
        out.points_.push_back(m.apply(p));
    }
    return out;
}

namespace {

constexpr double quarter_circle_handle = 0.5522847498307936;

}

Path rounded_rect_path(const Rect& rect, const CornerRadii& radii) {
    const CornerRadii r = fit_radii(rect, radii);
    const double l = rect.x;
    const double t = rect.y;
    const double rt = rect.x + rect.width;
    const double b = rect.y + rect.height;
    const double u = 1.0 - quarter_circle_handle;

    Path p;
    p.move_to({l + r.top_left, t});
    p.line_to({rt - r.top_right, t});
    if (r.top_right > 0.0) {
        p.cubic_to({rt - r.top_right * u, t}, {rt, t + r.top_right * u}, {rt, t + r.top_right});
    }
    p.line_to({rt, b - r.bottom_right});
    if (r.bottom_right > 0.0) {
        p.cubic_to({rt, b - r.bottom_right * u}, {rt - r.bottom_right * u, b}, {rt - r.bottom_right, b});
    }
    p.line_to({l + r.bottom_left, b});
    if (r.bottom_left > 0.0) {
        p.cubic_to({l + r.bottom_left * u, b}, {l, b - r.bottom_left * u}, {l, b - r.bottom_left});
    }
    p.line_to({l, t + r.top_left});
    if (r.top_left > 0.0) {
        p.cubic_to({l, t + r.top_left * u}, {l + r.top_left * u, t}, {l + r.top_left, t});
    }
    p.close();
    return p;
}

Path ring_path(const Rect& r, double radius, double width) {
    Path p = rounded_rect_path(r, uniform_radii(radius));
    const Rect inner{r.x + width, r.y + width, r.width - 2 * width, r.height - 2 * width};
    p.append(rounded_rect_path(inner, uniform_radii(std::max(0.0, radius - width))));
    return p;
}

Path circle_path(Point center, double radius) {
    return rounded_rect_path(circle_rect(center, radius), uniform_radii(radius));
}

namespace {

Point tangent_at(std::span<const Point> c, std::size_t i) {
    const std::size_t a = i == 0 ? 0 : i - 1;
    const std::size_t b = i + 1 == c.size() ? i : i + 1;
    return unit(c[b] - c[a]);
}

void round_cap(Path& p, Point center, Point normal, Point forward, double h) {
    const double k = quarter_circle_handle * h;
    const Point tip = center + forward * h;
    const Point end = center - normal * h;
    p.cubic_to(center + normal * h + forward * k, tip + normal * k, tip);
    p.cubic_to(tip - normal * k, end + forward * k, end);
}

}

Path stroke_path(std::span<const Point> c, double width) {
    Path p;
    const double h = width / 2.0;
    if (c.empty() || h <= 0.0) {
        return p;
    }
    if (c.size() == 1) {
        return circle_path(c[0], h);
    }
    std::vector<Point> normals;
    normals.reserve(c.size());
    for (std::size_t i = 0; i < c.size(); ++i) {
        normals.push_back(left_normal(tangent_at(c, i)));
    }
    p.move_to(c[0] + normals[0] * h);
    for (std::size_t i = 1; i < c.size(); ++i) {
        p.line_to(c[i] + normals[i] * h);
    }
    const std::size_t last = c.size() - 1;
    round_cap(p, c[last], normals[last], tangent_at(c, last), h);
    for (std::size_t i = last; i-- > 0;) {
        p.line_to(c[i] - normals[i] * h);
    }
    round_cap(p, c[0], -normals[0], -tangent_at(c, 0), h);
    p.close();
    return p;
}

}
