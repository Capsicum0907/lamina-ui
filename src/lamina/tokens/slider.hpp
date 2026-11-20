#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SliderTokens {
    double track_height;
    double handle_width;
    double handle_height;
    double pressed_handle_width;
    double handle_track_gap;
    double track_inside_corner;
    double stop_indicator_size;
    double tick_size;
    int keyboard_steps_when_continuous;
    int keyboard_page_divisor;
    int keyboard_page_max_steps;
    color::Role active_track;
    color::Role inactive_track;
    color::Role handle;
    color::Role active_tick;
    color::Role inactive_tick;
    color::Role stop_indicator;
    StateColor disabled_active_track;
    StateColor disabled_inactive_track;
    StateColor disabled_handle;
    StateColor disabled_active_tick;
    StateColor disabled_inactive_tick;
    StateColor disabled_stop_indicator;
    color::Role value_indicator_container;
    color::Role value_indicator_label;
    TypeRole value_indicator_type;
    double value_indicator_bottom_space;
    double value_indicator_padding;
    double value_indicator_min_size;
    motion::TweenSpec value_indicator_scale;
};

inline constexpr SliderTokens slider_tokens{
    .track_height = 16.0,
    .handle_width = 4.0,
    .handle_height = 44.0,
    .pressed_handle_width = 2.0,
    .handle_track_gap = 6.0,
    .track_inside_corner = 2.0,
    .stop_indicator_size = 4.0,
    .tick_size = 4.0,
    .keyboard_steps_when_continuous = 100,
    .keyboard_page_divisor = 10,
    .keyboard_page_max_steps = 10,
    .active_track = color::Role::primary,
    .inactive_track = color::Role::secondary_container,
    .handle = color::Role::primary,
    .active_tick = color::Role::secondary_container,
    .inactive_tick = color::Role::primary,
    .stop_indicator = color::Role::primary,
    .disabled_active_track = {color::Role::on_surface, 0.38},
    .disabled_inactive_track = {color::Role::on_surface, 0.12},
    .disabled_handle = {color::Role::on_surface, 0.38, true},
    .disabled_active_tick = {color::Role::on_surface, 0.12},
    .disabled_inactive_tick = {color::Role::on_surface, 0.38},
    .disabled_stop_indicator = {color::Role::on_surface, 0.38},
    .value_indicator_container = color::Role::inverse_surface,
    .value_indicator_label = color::Role::inverse_on_surface,
    .value_indicator_type = TypeRole::label_large,
    .value_indicator_bottom_space = 12.0,
    .value_indicator_padding = 4.0,
    .value_indicator_min_size = 28.0,
    .value_indicator_scale = {duration_ms::short_[1] / 1000.0, easing::emphasized},
};

}
