#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SnackbarTokens {
    color::Role container;
    CornerSize corner;
    std::size_t elevation;
    color::Role supporting_text_color;
    TypeRole supporting_text;
    color::Role action_color;
    ButtonStyle action_style;
    ButtonSize action_size;
    double single_line_height;
    double two_lines_height;
    double max_width;
    double outer_padding;
    double start_padding;
    double end_padding;
    double text_end_spacing;
    double text_vertical_padding;
    double short_duration_seconds;
    double long_duration_seconds;
    double closed_scale;
};

inline constexpr SnackbarTokens snackbar_tokens{
    .container = color::Role::inverse_surface,
    .corner = CornerSize::extra_small,
    .elevation = 3,
    .supporting_text_color = color::Role::inverse_on_surface,
    .supporting_text = TypeRole::body_medium,
    .action_color = color::Role::inverse_primary,
    .action_style = ButtonStyle::text,
    .action_size = ButtonSize::small,
    .single_line_height = 48.0,
    .two_lines_height = 68.0,
    .max_width = 600.0,
    .outer_padding = 12.0,
    .start_padding = 16.0,
    .end_padding = 8.0,
    .text_end_spacing = 8.0,
    .text_vertical_padding = 14.0,
    .short_duration_seconds = 4.0,
    .long_duration_seconds = 10.0,
    .closed_scale = 0.8,
};

}
