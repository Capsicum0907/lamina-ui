#include "lamina/render/path.hpp"

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

}
