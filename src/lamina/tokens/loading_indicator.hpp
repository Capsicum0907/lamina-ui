#pragma once

#include <array>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/shapes/material_shapes.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct LoadingIndicatorTokens {
    double container_size;
    double active_size;
    CornerSize container_shape;
    color::Role indicator;
    color::Role contained_indicator;
    color::Role contained_container;
    motion::SpringSpec morph_spring;
    motion::SpringSpec handoff_spring;
    double morph_threshold;
    double morph_interval_seconds;
    double rotation_period_seconds;
    double morph_turn_degrees;
    double determinate_turn_degrees;
    double determinate_circle_rotation_degrees;
    double estimate_step_seconds;
    double estimate_limit_seconds;
};

inline constexpr LoadingIndicatorTokens loading_indicator_tokens{
    .container_size = 48.0,
    .active_size = 38.0,
    .container_shape = CornerSize::full,
    .indicator = color::Role::primary,
    .contained_indicator = color::Role::on_primary_container,
    .contained_container = color::Role::primary_container,
    .morph_spring = {0.6, 200.0},
    .handoff_spring = motion_scheme.default_effects(),
    .morph_threshold = 0.1,
    .morph_interval_seconds = 0.65,
    .rotation_period_seconds = 4.666,
    .morph_turn_degrees = 90.0,
    .determinate_turn_degrees = -180.0,
    .determinate_circle_rotation_degrees = 360.0 / 20.0,
    .estimate_step_seconds = 0.001,
    .estimate_limit_seconds = 10.0,
};

inline constexpr std::array<shapes::MaterialShape, 7> loading_indicator_indeterminate_shapes{
    shapes::MaterialShape::soft_burst, shapes::MaterialShape::cookie_9_sided, shapes::MaterialShape::pentagon,
    shapes::MaterialShape::pill,       shapes::MaterialShape::sunny,          shapes::MaterialShape::cookie_4_sided,
    shapes::MaterialShape::oval,
};

inline constexpr std::array<shapes::MaterialShape, 2> loading_indicator_determinate_shapes{
    shapes::MaterialShape::circle,
    shapes::MaterialShape::soft_burst,
};

struct LoadingIndicatorSet {
    LoadingIndicatorTokens common = loading_indicator_tokens;
    std::array<shapes::MaterialShape, 7> indeterminate_shapes = loading_indicator_indeterminate_shapes;
    std::array<shapes::MaterialShape, 2> determinate_shapes = loading_indicator_determinate_shapes;
};

}
