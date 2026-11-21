#pragma once

#include <array>
#include <cstddef>

#include "lamina/render/geometry.hpp"

namespace lamina::tokens {

enum class CornerSize : std::size_t {
    none,
    extra_small,
    small,
    medium,
    large,
    large_increased,
    extra_large,
    extra_large_increased,
    extra_extra_large,
    full,
    count,
};

inline constexpr std::array<double, static_cast<std::size_t>(CornerSize::count) - 1> corner_radius_dp{
    0.0, 4.0, 8.0, 12.0, 16.0, 20.0, 28.0, 32.0, 48.0,
};

constexpr double corner_radius(CornerSize size, double width, double height) {
    if (size == CornerSize::full) {
        return (width < height ? width : height) / 2.0;
    }
    return corner_radius_dp[static_cast<std::size_t>(size)];
}

struct Corners {
    CornerSize top_start;
    CornerSize top_end;
    CornerSize bottom_end;
    CornerSize bottom_start;

    friend constexpr bool operator==(const Corners&, const Corners&) = default;
};

constexpr Corners all_corners(CornerSize size) {
    return {size, size, size, size};
}

constexpr render::CornerRadii corner_radii(const Corners& c, double width, double height) {
    return {corner_radius(c.top_start, width, height), corner_radius(c.top_end, width, height),
            corner_radius(c.bottom_end, width, height), corner_radius(c.bottom_start, width, height)};
}

namespace corners {
inline constexpr Corners extra_small_top{CornerSize::extra_small, CornerSize::extra_small, CornerSize::none, CornerSize::none};
inline constexpr Corners large_top{CornerSize::large, CornerSize::large, CornerSize::none, CornerSize::none};
inline constexpr Corners large_start{CornerSize::large, CornerSize::none, CornerSize::none, CornerSize::large};
inline constexpr Corners large_end{CornerSize::none, CornerSize::large, CornerSize::large, CornerSize::none};
inline constexpr Corners extra_large_top{CornerSize::extra_large, CornerSize::extra_large, CornerSize::none, CornerSize::none};
}

}
