#pragma once

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct TimePickerTokens {
    double dial_size;
    double outer_ratio;
    double inner_ratio;
    double inner_ring_distance;
    double label_size;
    double handle_size;
    double track_width;
    double center_size;
    TypeRole dial_label_type;
    double selector_width;
    double selector_height;
    CornerSize selector_corner;
    TypeRole selector_type;
    double separator_width;
    double separator_offset;
    double period_vertical_width;
    double period_vertical_height;
    double period_horizontal_width;
    double period_horizontal_height;
    double period_gap;
    double period_start_padding;
    double period_top_padding;
    double period_checked_corner;
    TypeRole period_type;
    int period_selected_weight;
    double display_bottom;
    double dial_bottom;
    double input_field_width;
    double input_field_height;
    CornerSize input_field_corner;
    TypeRole input_type;
    double input_outline;
    double input_focus_outline;
    double input_supporting_top;
    int input_supporting_lines;
    TypeRole input_supporting_type;
    double caret_width;
    double caret_inset;
    double auto_switch_delay_seconds;
    double touch_slop;
    int minute_tap_snap;
    icons::Symbol keyboard_symbol;
    icons::Symbol clock_symbol;
};

inline constexpr TimePickerTokens time_picker_tokens{
    .dial_size = 256.0,
    .outer_ratio = 101.0 / 256.0,
    .inner_ratio = 69.0 / 256.0,
    .inner_ring_distance = 74.0,
    .label_size = 48.0,
    .handle_size = 48.0,
    .track_width = 2.0,
    .center_size = 8.0,
    .dial_label_type = TypeRole::body_large,
    .selector_width = 96.0,
    .selector_height = 80.0,
    .selector_corner = CornerSize::small,
    .selector_type = TypeRole::display_large,
    .separator_width = 24.0,
    .separator_offset = -4.0,
    .period_vertical_width = 52.0,
    .period_vertical_height = 80.0,
    .period_horizontal_width = 216.0,
    .period_horizontal_height = 38.0,
    .period_gap = 4.0,
    .period_start_padding = 4.0,
    .period_top_padding = 16.0,
    .period_checked_corner = 12.0,
    .period_type = TypeRole::title_medium,
    .period_selected_weight = 700,
    .display_bottom = 36.0,
    .dial_bottom = 24.0,
    .input_field_width = 96.0,
    .input_field_height = 72.0,
    .input_field_corner = CornerSize::small,
    .input_type = TypeRole::display_medium,
    .input_outline = 1.0,
    .input_focus_outline = 2.0,
    .input_supporting_top = 7.0,
    .input_supporting_lines = 2,
    .input_supporting_type = TypeRole::body_small,
    .caret_width = 2.0,
    .caret_inset = 0.1,
    .auto_switch_delay_seconds = 0.1,
    .touch_slop = 8.0,
    .minute_tap_snap = 5,
    .keyboard_symbol = builtin::icons::keyboard,
    .clock_symbol = builtin::icons::schedule,
};

struct TimePickerColors {
    color::Role dial;
    color::Role dial_content;
    color::Role dial_selected_content;
    color::Role selector;
    color::Role period_border;
    color::Role period_selected_container;
    color::Role period_container;
    color::Role period_selected_content;
    color::Role period_content;
    color::Role time_selected_container;
    color::Role time_container;
    color::Role time_selected_content;
    color::Role time_content;
    color::Role separator;
    color::Role input_outline;
    color::Role supporting;
    color::Role error;
    color::Role error_container;
    color::Role on_error_container;
    double disabled_opacity;
};

inline constexpr TimePickerColors time_picker_colors{
    .dial = color::Role::surface_container_highest,
    .dial_content = color::Role::on_surface,
    .dial_selected_content = color::Role::on_primary,
    .selector = color::Role::primary,
    .period_border = color::Role::outline,
    .period_selected_container = color::Role::primary_container,
    .period_container = color::Role::surface_container_lowest,
    .period_selected_content = color::Role::on_primary_container,
    .period_content = color::Role::on_surface_variant,
    .time_selected_container = color::Role::primary_container,
    .time_container = color::Role::surface_container_highest,
    .time_selected_content = color::Role::on_primary_container,
    .time_content = color::Role::on_surface,
    .separator = color::Role::on_surface,
    .input_outline = color::Role::outline,
    .supporting = color::Role::on_surface_variant,
    .error = color::Role::error,
    .error_container = color::Role::error_container,
    .on_error_container = color::Role::on_error_container,
    .disabled_opacity = disabled_alpha,
};

struct TimePickerMotion {
    motion::SpringSpec selector;
    motion::SpringSpec numbers;
    motion::SpringSpec period_shape;
    motion::SpringSpec colors;
    motion::SpringSpec state_layer;
    motion::SpringSpec mode;
};

inline constexpr TimePickerMotion time_picker_motion{
    .selector = motion_scheme.default_spatial(),
    .numbers = motion_scheme.default_effects(),
    .period_shape = motion_scheme.fast_spatial(),
    .colors = motion_scheme.default_effects(),
    .state_layer = motion_scheme.fast_effects(),
    .mode = motion_scheme.fast_effects(),
};

}
