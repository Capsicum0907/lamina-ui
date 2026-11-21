#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct NumericFieldTokens {
    DenseLength height;
    double width;
    double label_gap;
    double padding;
    double unit_gap;
    double inline_label_gap;
    CornerSize corner;
    double outline_width;
    double focus_outline_width;
    TypeRole label_type;
    color::Role label;
    color::Role unit;
    color::Role outline;
    color::Role hover_outline;
    color::Role focus_outline;
    color::Role inline_container;
    color::Role error;
    StateColor disabled_content;
    StateColor disabled_outline;
    double scrub_distance;
    double scrub_slop;
    double state_layer;
    motion::SpringSpec color_spring;
};

inline constexpr NumericFieldTokens numeric_field_tokens{
    .height = {40.0, 28.0},
    .width = 96.0,
    .label_gap = 8.0,
    .padding = 8.0,
    .unit_gap = 4.0,
    .inline_label_gap = 6.0,
    .corner = CornerSize::extra_small,
    .outline_width = 1.0,
    .focus_outline_width = 2.0,
    .label_type = TypeRole::label_large,
    .label = color::Role::on_surface_variant,
    .unit = color::Role::on_surface_variant,
    .outline = color::Role::outline,
    .hover_outline = color::Role::on_surface,
    .focus_outline = color::Role::primary,
    .inline_container = color::Role::surface_container_highest,
    .error = color::Role::error,
    .disabled_content = {color::Role::on_surface, 0.38},
    .disabled_outline = {color::Role::on_surface, 0.12},
    .scrub_distance = 4.0,
    .scrub_slop = 3.0,
    .state_layer = 0.08,
    .color_spring = motion_scheme.default_effects(),
};

}
