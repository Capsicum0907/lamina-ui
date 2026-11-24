#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/motion/tween.hpp"
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

struct PullArrowTokens {
    double container_size;
    color::Role container;
    color::Role indicator;
    std::size_t elevation_level;
    double stroke;
    double arc_radius;
    double arrow_width;
    double arrow_height;
    double max_arc;
    double arc_start;
    double arc_gain;
    double rotation_start;
    double rotation_arc;
    double rotation_scale;
    double max_tension;
    double min_alpha;
    double max_alpha;
    motion::TweenSpec alpha_tween;
    motion::TweenSpec crossfade;
    double spinner_size;
    int arc_segments_per_turn;
};

inline constexpr PullArrowTokens pull_arrow_tokens{
    .container_size = 40.0,
    .container = color::Role::surface_container_high,
    .indicator = color::Role::on_surface_variant,
    .elevation_level = 2,
    .stroke = 2.5,
    .arc_radius = 5.5,
    .arrow_width = 10.0,
    .arrow_height = 5.0,
    .max_arc = 0.8,
    .arc_start = 0.4,
    .arc_gain = 5.0 / 3.0,
    .rotation_start = -0.25,
    .rotation_arc = 0.4,
    .rotation_scale = 0.5,
    .max_tension = 2.0,
    .min_alpha = 0.3,
    .max_alpha = 1.0,
    .alpha_tween = {0.3, easing::linear},
    .crossfade = {0.1, easing::linear},
    .spinner_size = 16.0,
    .arc_segments_per_turn = 96,
};

}
