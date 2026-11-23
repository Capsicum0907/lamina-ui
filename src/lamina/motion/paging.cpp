#include "lamina/motion/paging.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::motion {

namespace {

std::size_t clamp_page(double page, std::size_t count) {
    if (count == 0) {
        return 0;
    }
    return static_cast<std::size_t>(std::clamp(page, 0.0, static_cast<double>(count - 1)));
}

}

double clamp_position(double position, std::size_t count) {
    return count == 0 ? 0.0 : std::clamp(position, 0.0, static_cast<double>(count - 1));
}

std::size_t nearest_page(double position, std::size_t count) {
    return clamp_page(std::floor(clamp_position(position, count) + 0.5), count);
}

std::size_t snap_page(double position, std::size_t settled, double drag_distance, double velocity, std::size_t count, const PageSnapSpec& spec) {
    const double p = clamp_position(position, count);
    const double first_visible = std::floor(p);
    if (std::abs(drag_distance) > spec.min_fling_distance && std::abs(velocity) > spec.min_fling_velocity) {
        return clamp_page(velocity < 0.0 ? first_visible + 1.0 : first_visible, count);
    }
    const double truncator = p >= static_cast<double>(settled) ? 1.0 - spec.positional_threshold : spec.positional_threshold;
    return clamp_page(std::floor(p + truncator), count);
}

PageRange pages_in_view(double position, std::size_t count) {
    if (count == 0) {
        return {};
    }
    const double p = clamp_position(position, count);
    return {clamp_page(std::floor(p), count), clamp_page(std::ceil(p), count), false};
}

PageRange pages_around(double position, std::size_t count, std::size_t beyond) {
    PageRange r = pages_in_view(position, count);
    if (r.empty) {
        return r;
    }
    r.first = r.first > beyond ? r.first - beyond : 0;
    r.last = std::min(count - 1, r.last + beyond);
    return r;
}

}
