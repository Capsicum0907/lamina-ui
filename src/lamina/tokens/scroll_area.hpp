#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::tokens {

struct ScrollAreaTokens {
    double wheel_step;
    double page_overlap;
    double bar_thickness;
    double hovered_bar_thickness;
    double bar_margin;
    double bar_hit_width;
    double min_thumb_length;
    double bar_opacity;
    double hovered_bar_opacity;
    double hide_delay_seconds;
    color::Role bar;
    motion::SpringSpec offset_spring;
    motion::SpringSpec bar_spring;
};

inline constexpr ScrollAreaTokens scroll_area_tokens{
    .wheel_step = 64.0,
    .page_overlap = 48.0,
    .bar_thickness = 4.0,
    .hovered_bar_thickness = 8.0,
    .bar_margin = 2.0,
    .bar_hit_width = 12.0,
    .min_thumb_length = 16.0,
    .bar_opacity = 0.38,
    .hovered_bar_opacity = 0.5,
    .hide_delay_seconds = 1.0,
    .bar = color::Role::on_surface,
    .offset_spring = motion_scheme.default_effects(),
    .bar_spring = motion_scheme.fast_effects(),
};

}
