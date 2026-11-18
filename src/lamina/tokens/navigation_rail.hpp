#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct NavigationRailTokens {
    double collapsed_width;
    double expanded_min_width;
    double expanded_max_width;
    double top_space;
    double header_space;
    double collapsed_item_space;
    double expanded_item_space;
    double collapsed_item_min_height;
    double expanded_item_min_height;
    double item_min_width;
    double item_horizontal_padding;
    double icon_size;
    double top_indicator_width;
    double top_indicator_height;
    double start_indicator_height;
    double start_indicator_leading_space;
    double top_icon_label_space;
    double start_icon_label_space;
    double label_switch_progress;
    TypeRole top_label;
    TypeRole start_label;
    color::Role container;
    color::Role indicator;
    color::Role active_icon;
    color::Role top_active_label;
    color::Role start_active_label;
    color::Role inactive_icon;
    color::Role inactive_label;
    color::Role state_layer;
};

inline constexpr NavigationRailTokens navigation_rail_tokens{
    .collapsed_width = 96.0,
    .expanded_min_width = 220.0,
    .expanded_max_width = 360.0,
    .top_space = 44.0,
    .header_space = 40.0,
    .collapsed_item_space = 4.0,
    .expanded_item_space = 0.0,
    .collapsed_item_min_height = 64.0,
    .expanded_item_min_height = minimum_interactive_size,
    .item_min_width = minimum_interactive_size,
    .item_horizontal_padding = 20.0,
    .icon_size = 24.0,
    .top_indicator_width = 56.0,
    .top_indicator_height = 32.0,
    .start_indicator_height = 56.0,
    .start_indicator_leading_space = 16.0,
    .top_icon_label_space = 4.0,
    .start_icon_label_space = 8.0,
    .label_switch_progress = 0.5,
    .top_label = TypeRole::label_medium,
    .start_label = TypeRole::label_large,
    .container = color::Role::surface,
    .indicator = color::Role::secondary_container,
    .active_icon = color::Role::on_secondary_container,
    .top_active_label = color::Role::secondary,
    .start_active_label = color::Role::on_secondary_container,
    .inactive_icon = color::Role::on_surface_variant,
    .inactive_label = color::Role::on_surface_variant,
    .state_layer = color::Role::on_surface,
};

}
