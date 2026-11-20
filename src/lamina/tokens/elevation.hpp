#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "lamina/motion/tween.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::tokens {

inline constexpr std::array<double, 6> elevation_dp{0.0, 1.0, 3.0, 6.0, 8.0, 12.0};

struct ShadowLayer {
    double offset_y;
    double blur;
    double spread;
    double opacity;
};

inline constexpr double key_shadow_opacity = 0.3;
inline constexpr double ambient_shadow_opacity = 0.15;
inline constexpr double css_blur_per_sigma = 2.0;

constexpr double level_for_elevation(double dp) {
    if (dp <= elevation_dp.front()) {
        return 0.0;
    }
    for (std::size_t i = 1; i < elevation_dp.size(); ++i) {
        if (dp <= elevation_dp[i]) {
            return static_cast<double>(i - 1) + (dp - elevation_dp[i - 1]) / (elevation_dp[i] - elevation_dp[i - 1]);
        }
    }
    return static_cast<double>(elevation_dp.size() - 1);
}

constexpr double clamp_step(double v, double hi) {
    return std::clamp(v, 0.0, hi);
}

constexpr ShadowLayer key_shadow(double level) {
    const double y = clamp_step(level, 1) + clamp_step(level - 3, 1) + 2 * clamp_step(level - 4, 1);
    const double blur = 2 * clamp_step(level, 1) + clamp_step(level - 2, 1) + clamp_step(level - 4, 1);
    return {y, blur, 0.0, key_shadow_opacity};
}

constexpr ShadowLayer ambient_shadow(double level) {
    const double y = clamp_step(level, 1) + clamp_step(level - 1, 1) + 2 * clamp_step(level - 2, 3);
    const double blur = 3 * clamp_step(level, 2) + 2 * clamp_step(level - 2, 3);
    const double spread = clamp_step(level, 4) + 2 * clamp_step(level - 4, 1);
    return {y, blur, spread, ambient_shadow_opacity};
}

struct StateElevations {
    double resting;
    double hovered;
    double focused;
    double pressed;
    double disabled;
};

struct ElevationTweens {
    motion::TweenSpec incoming;
    motion::TweenSpec outgoing;
    motion::TweenSpec hover_outgoing;
};

inline constexpr ElevationTweens elevation_tweens{
    .incoming = {0.12, easing::legacy},
    .outgoing = {0.15, {0.4, 0.0, 0.6, 1.0}},
    .hover_outgoing = {0.12, {0.4, 0.0, 0.6, 1.0}},
};

}
