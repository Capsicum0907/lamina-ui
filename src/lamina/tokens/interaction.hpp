#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

inline constexpr double minimum_interactive_size = 48.0;

inline constexpr double touch_echo_tolerance_pixels = 1.0;

namespace state_layer_opacity {
inline constexpr double hover = 0.08;
inline constexpr double focus = 0.10;
inline constexpr double pressed = 0.10;
inline constexpr double dragged = 0.16;
}

inline constexpr double disabled_alpha = 0.38;

struct StateColor {
    color::Role role;
    double opacity = 1.0;
    bool over_surface = false;
    std::optional<color::Role> over = std::nullopt;
};

}
