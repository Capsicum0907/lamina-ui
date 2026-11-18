#include "lamina/shapes/point.hpp"

#include <stdexcept>

namespace lamina::shapes {

Point get_direction(Point p) {
    const float d = get_distance(p);
    if (!(d > 0.0f)) {
        throw std::invalid_argument("Can't get the direction of a 0-length vector");
    }
    return p / d;
}

Point direction_vector(float x, float y) {
    const float d = distance(x, y);
    if (!(d > 0.0f)) {
        throw std::invalid_argument("Required distance greater than zero");
    }
    return {x / d, y / d};
}

bool convex(Point previous, Point current, Point next) {
    return clockwise(current - previous, next - current);
}

}
