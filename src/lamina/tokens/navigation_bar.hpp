#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct NavigationBarTokens {
    double height;
    double indicator_width;
    double indicator_height;
    double icon_size;
    double item_vertical_padding;
    double indicator_to_label_space;
    TypeRole label;
    color::Role container;
    color::Role indicator;
    color::Role active_icon;
    color::Role active_label;
    color::Role inactive_icon;
    color::Role inactive_label;
    color::Role state_layer;
    StateColor disabled_icon;
    StateColor disabled_label;
};

inline constexpr NavigationBarTokens navigation_bar_tokens{
    .height = 64.0,
    .indicator_width = 56.0,
    .indicator_height = 32.0,
    .icon_size = 24.0,
    .item_vertical_padding = 6.0,
    .indicator_to_label_space = 4.0,
    .label = TypeRole::label_medium,
    .container = color::Role::surface_container,
    .indicator = color::Role::secondary_container,
    .active_icon = color::Role::on_secondary_container,
    .active_label = color::Role::secondary,
    .inactive_icon = color::Role::on_surface_variant,
    .inactive_label = color::Role::on_surface_variant,
    .state_layer = color::Role::on_surface,
    .disabled_icon = {color::Role::on_surface_variant, disabled_alpha},
    .disabled_label = {color::Role::on_surface_variant, disabled_alpha},
};

}
