#pragma once

#include <cstddef>

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct MenuTokens {
    color::Role container;
    CornerSize corner;
    std::size_t elevation;
    double vertical_padding;
    double item_height;
    double item_min_width;
    double item_max_width;
    double item_horizontal_padding;
    double icon_size;
    double icon_text_space;
    TypeRole label;
    color::Role label_color;
    color::Role leading_icon;
    color::Role state_layer;
    double window_vertical_margin;
    double window_horizontal_margin;
    double closed_scale;
    double focus_ring_corner;
    StateColor disabled_label;
    StateColor disabled_leading_icon;
    double trailing_space;
    double trailing_icon_size;
    color::Role trailing;
    StateColor disabled_trailing;
    icons::Symbol submenu_symbol;
    double divider_thickness;
    color::Role divider_color;
    double divider_horizontal_padding;
    double divider_vertical_padding;
    double submenu_open_seconds;
    double submenu_close_seconds;
    double context_window_margin;
};

inline constexpr MenuTokens menu_tokens{
    .container = color::Role::surface_container,
    .corner = CornerSize::extra_small,
    .elevation = 2,
    .vertical_padding = 8.0,
    .item_height = 48.0,
    .item_min_width = 112.0,
    .item_max_width = 280.0,
    .item_horizontal_padding = 12.0,
    .icon_size = 24.0,
    .icon_text_space = 12.0,
    .label = TypeRole::label_large,
    .label_color = color::Role::on_surface,
    .leading_icon = color::Role::on_surface_variant,
    .state_layer = color::Role::on_surface,
    .window_vertical_margin = 48.0,
    .window_horizontal_margin = 0.0,
    .closed_scale = 0.8,
    .focus_ring_corner = 0.0,
    .disabled_label = {color::Role::on_surface, 0.38},
    .disabled_leading_icon = {color::Role::on_surface, 0.38},
    .trailing_space = 12.0,
    .trailing_icon_size = 24.0,
    .trailing = color::Role::on_surface_variant,
    .disabled_trailing = {color::Role::on_surface, 0.38},
    .submenu_symbol = builtin::icons::keyboard_arrow_right,
    .divider_thickness = 1.0,
    .divider_color = color::Role::outline_variant,
    .divider_horizontal_padding = 12.0,
    .divider_vertical_padding = 2.0,
    .submenu_open_seconds = 0.4,
    .submenu_close_seconds = 0.4,
    .context_window_margin = 4.0,
};

}
