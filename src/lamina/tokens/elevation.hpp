#pragma once

#include <array>

#include "lamina/color/mix.hpp"
#include "lamina/render/canvas.hpp"

namespace lamina::tokens {

inline constexpr std::array<double, 6> elevation_dp{0.0, 1.0, 3.0, 6.0, 8.0, 12.0};

inline constexpr double shadow_opacity = 0.3;

inline render::Shadow shadow_for_elevation(double elevation, color::Argb shadow_color) {
    return {0.0, elevation, elevation, color::with_alpha(shadow_color, shadow_opacity)};
}

namespace state_layer_opacity {
inline constexpr double hover = 0.08;
inline constexpr double focus = 0.10;
inline constexpr double pressed = 0.10;
inline constexpr double dragged = 0.16;
}

}
