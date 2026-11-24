#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/tokens/loading_indicator.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct PullToRefreshTokens {
    double threshold;
    double drag_multiplier;
    double max_tension;
    double container_size;
    CornerSize container_shape;
    std::size_t elevation_level;
    double start_scale;
    double overshoot_turn_degrees;
    double slop;
    double max_angle_degrees;
    double top_tolerance;
    double min_refresh_seconds;
    color::Role container;
    color::Role indicator;
    color::Role shadow;
    motion::SpringSpec settle_spring;
};

inline constexpr PullToRefreshTokens pull_to_refresh_tokens{
    .threshold = 80.0,
    .drag_multiplier = 0.5,
    .max_tension = 2.0,
    .container_size = loading_indicator_tokens.container_size,
    .container_shape = CornerSize::full,
    .elevation_level = 2,
    .start_scale = 0.5,
    .overshoot_turn_degrees = 180.0,
    .slop = touch_slop,
    .max_angle_degrees = 45.0,
    .top_tolerance = 0.5,
    .min_refresh_seconds = 0.6,
    .container = loading_indicator_tokens.contained_container,
    .indicator = loading_indicator_tokens.contained_indicator,
    .shadow = color::Role::shadow,
    .settle_spring = motion_scheme.default_spatial(),
};

}
