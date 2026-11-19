#include "lamina/render/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::render {

double length(Point v) {
    return std::hypot(v.x, v.y);
}

Point unit(Point v) {
    const double l = length(v);
    return l > 0.0 ? Point{v.x / l, v.y / l} : Point{0.0, 0.0};
}

CornerRadii fit_radii(const Rect& rect, CornerRadii r) {
    r.top_left = std::max(0.0, r.top_left);
    r.top_right = std::max(0.0, r.top_right);
    r.bottom_right = std::max(0.0, r.bottom_right);
    r.bottom_left = std::max(0.0, r.bottom_left);

    double factor = 1.0;
    auto limit = [&factor](double sum, double side) {
        if (sum > side && sum > 0.0) {
            factor = std::min(factor, side / sum);
        }
    };
    limit(r.top_left + r.top_right, rect.width);
    limit(r.bottom_left + r.bottom_right, rect.width);
    limit(r.top_left + r.bottom_left, rect.height);
    limit(r.top_right + r.bottom_right, rect.height);

    r.top_left *= factor;
    r.top_right *= factor;
    r.bottom_right *= factor;
    r.bottom_left *= factor;
    return r;
}

}
