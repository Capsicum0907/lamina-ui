#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct DialogTokens {
    color::Role container;
    CornerSize corner;
    double padding;
    double headline_bottom_space;
    double supporting_text_bottom_space;
    double min_width;
    double max_width;
    double button_space;
    double button_line_space;
    double icon_size;
    color::Role icon_color;
    double icon_bottom_space;
    TypeRole headline;
    color::Role headline_color;
    TypeRole supporting_text;
    color::Role supporting_text_color;
    ButtonStyle action_style;
    ButtonSize action_size;
    color::Role scrim;
    double scrim_opacity;
    double closed_scale;
};

inline constexpr DialogTokens dialog_tokens{
    .container = color::Role::surface_container_high,
    .corner = CornerSize::extra_large,
    .padding = 24.0,
    .headline_bottom_space = 16.0,
    .supporting_text_bottom_space = 24.0,
    .min_width = 280.0,
    .max_width = 560.0,
    .button_space = 8.0,
    .button_line_space = 8.0,
    .icon_size = 24.0,
    .icon_color = color::Role::secondary,
    .icon_bottom_space = 16.0,
    .headline = TypeRole::headline_small,
    .headline_color = color::Role::on_surface,
    .supporting_text = TypeRole::body_medium,
    .supporting_text_color = color::Role::on_surface_variant,
    .action_style = ButtonStyle::text,
    .action_size = ButtonSize::small,
    .scrim = color::Role::scrim,
    .scrim_opacity = 0.32,
    .closed_scale = 0.8,
};

}
