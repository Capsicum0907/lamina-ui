#pragma once

#include <array>

namespace lamina::tokens {

inline constexpr std::array<double, 6> elevation_dp{0.0, 1.0, 3.0, 6.0, 8.0, 12.0};

namespace state_layer_opacity {
inline constexpr double hover = 0.08;
inline constexpr double focus = 0.10;
inline constexpr double pressed = 0.10;
inline constexpr double dragged = 0.16;
}

}
