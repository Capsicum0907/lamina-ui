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

constexpr double clamp_step(double v, double hi) {
    return std::clamp(v, 0.0, hi);
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

struct ElevationTokens {
    std::array<double, 6> levels = elevation_dp;
    double key_opacity = key_shadow_opacity;
    double ambient_opacity = ambient_shadow_opacity;
    double blur_per_sigma = css_blur_per_sigma;
    ElevationTweens tweens = elevation_tweens;

    constexpr double level_for(double dp) const {
        if (dp <= levels.front()) {
            return 0.0;
        }
        for (std::size_t i = 1; i < levels.size(); ++i) {
            if (dp <= levels[i]) {
                return static_cast<double>(i - 1) + (dp - levels[i - 1]) / (levels[i] - levels[i - 1]);
            }
        }
        return static_cast<double>(levels.size() - 1);
    }

    constexpr ShadowLayer key_shadow(double level) const {
        const double y = clamp_step(level, 1) + clamp_step(level - 3, 1) + 2 * clamp_step(level - 4, 1);
        const double blur = 2 * clamp_step(level, 1) + clamp_step(level - 2, 1) + clamp_step(level - 4, 1);
        return {y, blur, 0.0, key_opacity};
    }

    constexpr ShadowLayer ambient_shadow(double level) const {
        const double y = clamp_step(level, 1) + clamp_step(level - 1, 1) + 2 * clamp_step(level - 2, 3);
        const double blur = 3 * clamp_step(level, 2) + 2 * clamp_step(level - 2, 3);
        const double spread = clamp_step(level, 4) + 2 * clamp_step(level - 4, 1);
        return {y, blur, spread, ambient_opacity};
    }
};

inline constexpr ElevationTokens elevation_tokens{};

constexpr double level_for_elevation(double dp) {
    return elevation_tokens.level_for(dp);
}

constexpr ShadowLayer key_shadow(double level) {
    return elevation_tokens.key_shadow(level);
}

constexpr ShadowLayer ambient_shadow(double level) {
    return elevation_tokens.ambient_shadow(level);
}

}
