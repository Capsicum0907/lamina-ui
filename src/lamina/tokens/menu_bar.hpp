#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct MenuBarTokens {
    color::Role container;
    double height;
    double start_padding;
    double item_vertical_inset;
    double item_horizontal_padding;
    CornerSize item_corner;
    TypeRole label;
    color::Role label_color;
    color::Role open_container;
    color::Role open_label;
    color::Role state_layer;
    StateColor disabled_label;
    double window_margin;
};

inline constexpr MenuBarTokens menu_bar_tokens{
    .container = color::Role::surface_container_low,
    .height = 40.0,
    .start_padding = 4.0,
    .item_vertical_inset = 4.0,
    .item_horizontal_padding = 12.0,
    .item_corner = CornerSize::small,
    .label = TypeRole::label_large,
    .label_color = color::Role::on_surface,
    .open_container = color::Role::secondary_container,
    .open_label = color::Role::on_secondary_container,
    .state_layer = color::Role::on_surface,
    .disabled_label = {color::Role::on_surface, 0.38},
    .window_margin = 4.0,
};

}
