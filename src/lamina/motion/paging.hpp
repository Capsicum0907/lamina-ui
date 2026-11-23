#pragma once

#include <cstddef>

namespace lamina::motion {

struct PageSnapSpec {
    double min_fling_velocity;
    double min_fling_distance;
    double positional_threshold;
};

struct PageRange {
    std::size_t first = 0;
    std::size_t last = 0;
    bool empty = true;

    bool contains(std::size_t page) const { return !empty && page >= first && page <= last; }
};

double clamp_position(double position, std::size_t count);
std::size_t nearest_page(double position, std::size_t count);
std::size_t snap_page(double position, std::size_t settled, double drag_distance, double velocity, std::size_t count, const PageSnapSpec& spec);
PageRange pages_in_view(double position, std::size_t count);
PageRange pages_around(double position, std::size_t count, std::size_t beyond);

}
