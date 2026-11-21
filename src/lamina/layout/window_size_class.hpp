#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace lamina::layout {

enum class WindowWidthClass : std::size_t { compact, medium, expanded, large, extra_large, count };
enum class WindowHeightClass : std::size_t { compact, medium, expanded, count };

struct WindowBreakpoints {
    std::array<int, static_cast<std::size_t>(WindowWidthClass::count)> width;
    std::array<int, static_cast<std::size_t>(WindowHeightClass::count)> height;
};

inline constexpr WindowBreakpoints window_breakpoints{
    .width = {0, 600, 840, 1200, 1600},
    .height = {0, 480, 900},
};

struct WindowSizeClass {
    int min_width_dp = 0;
    int min_height_dp = 0;

    constexpr bool is_width_at_least(int breakpoint_dp) const { return min_width_dp >= breakpoint_dp; }
    constexpr bool is_height_at_least(int breakpoint_dp) const { return min_height_dp >= breakpoint_dp; }
    constexpr bool is_width_at_least(WindowWidthClass c) const {
        return is_width_at_least(window_breakpoints.width[static_cast<std::size_t>(c)]);
    }
    constexpr bool is_height_at_least(WindowHeightClass c) const {
        return is_height_at_least(window_breakpoints.height[static_cast<std::size_t>(c)]);
    }

    constexpr WindowWidthClass width_class() const {
        std::size_t k = 0;
        for (std::size_t i = 0; i < window_breakpoints.width.size(); ++i) {
            if (min_width_dp >= window_breakpoints.width[i]) {
                k = i;
            }
        }
        return static_cast<WindowWidthClass>(k);
    }
    constexpr WindowHeightClass height_class() const {
        std::size_t k = 0;
        for (std::size_t i = 0; i < window_breakpoints.height.size(); ++i) {
            if (min_height_dp >= window_breakpoints.height[i]) {
                k = i;
            }
        }
        return static_cast<WindowHeightClass>(k);
    }

    constexpr bool operator==(const WindowSizeClass&) const = default;
};

constexpr WindowSizeClass compute_window_size_class(double width_dp, double height_dp) {
    const int w = width_dp > 0.0 ? static_cast<int>(width_dp) : 0;
    const int h = height_dp > 0.0 ? static_cast<int>(height_dp) : 0;
    WindowSizeClass c;
    for (int b : window_breakpoints.width) {
        if (b <= w && b > c.min_width_dp) {
            c.min_width_dp = b;
        }
    }
    for (int b : window_breakpoints.height) {
        if (b <= h && b > c.min_height_dp) {
            c.min_height_dp = b;
        }
    }
    return c;
}

constexpr std::string_view name(WindowWidthClass c) {
    constexpr std::array<std::string_view, static_cast<std::size_t>(WindowWidthClass::count)> names{"compact", "medium", "expanded", "large",
                                                                                                    "extra-large"};
    return names[static_cast<std::size_t>(c)];
}

constexpr std::string_view name(WindowHeightClass c) {
    constexpr std::array<std::string_view, static_cast<std::size_t>(WindowHeightClass::count)> names{"compact", "medium", "expanded"};
    return names[static_cast<std::size_t>(c)];
}

}
