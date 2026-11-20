#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct PlainTooltipTokens {
    color::Role container;
    color::Role content;
    TypeRole text;
    CornerSize corner;
    double max_width;
    double horizontal_padding;
    double vertical_padding;
};

inline constexpr PlainTooltipTokens plain_tooltip_tokens{
    .container = color::Role::inverse_surface,
    .content = color::Role::inverse_on_surface,
    .text = TypeRole::body_small,
    .corner = CornerSize::extra_small,
    .max_width = 200.0,
    .horizontal_padding = 8.0,
    .vertical_padding = 4.0,
};

struct RichTooltipTokens {
    color::Role container;
    color::Role title_color;
    color::Role text_color;
    color::Role action_color;
    TypeRole title;
    TypeRole text;
    CornerSize corner;
    std::size_t elevation;
    double max_width;
    double horizontal_padding;
    double title_first_baseline;
    double text_first_baseline;
    double text_bottom_padding;
    double plain_vertical_padding;
    double action_min_height;
    double action_bottom_padding;
    ButtonStyle action_style;
    ButtonSize action_size;
};

inline constexpr RichTooltipTokens rich_tooltip_tokens{
    .container = color::Role::surface_container,
    .title_color = color::Role::on_surface_variant,
    .text_color = color::Role::on_surface_variant,
    .action_color = color::Role::primary,
    .title = TypeRole::title_small,
    .text = TypeRole::body_medium,
    .corner = CornerSize::medium,
    .elevation = 2,
    .max_width = 320.0,
    .horizontal_padding = 16.0,
    .title_first_baseline = 28.0,
    .text_first_baseline = 24.0,
    .text_bottom_padding = 16.0,
    .plain_vertical_padding = 4.0,
    .action_min_height = 36.0,
    .action_bottom_padding = 8.0,
    .action_style = ButtonStyle::text,
    .action_size = ButtonSize::small,
};

struct TooltipTokens {
    double min_width;
    double min_height;
    double anchor_spacing;
    double caret_width;
    double caret_height;
    double closed_scale;
    double timeout_seconds;
    double hover_show_delay_seconds;
    double long_press_seconds;
};

inline constexpr TooltipTokens tooltip_tokens{
    .min_width = 40.0,
    .min_height = 24.0,
    .anchor_spacing = 4.0,
    .caret_width = 16.0,
    .caret_height = 8.0,
    .closed_scale = 0.8,
    .timeout_seconds = 1.5,
    .hover_show_delay_seconds = 0.0,
    .long_press_seconds = 0.5,
};

}
