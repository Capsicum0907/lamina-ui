#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"

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
};

}
