#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
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
};

struct GroupedMenuTokens {
    color::Role group_container;
    std::size_t elevation;
    Corners standalone_group;
    Corners leading_group;
    Corners middle_group;
    Corners trailing_group;
    CornerSize inactive_group;
    double group_gap;
    double group_vertical_padding;
    double item_horizontal_inset;
    double item_min_height;
    double item_min_width;
    double item_max_width;
    double item_horizontal_padding;
    double item_vertical_padding;
    double leading_icon_size;
    double trailing_icon_size;
    double icon_text_space;
    Corners standalone_item;
    Corners leading_item;
    Corners middle_item;
    Corners trailing_item;
    CornerSize selected_item;
    TypeRole label;
    color::Role label_color;
    color::Role leading_icon;
    color::Role trailing_icon;
    color::Role state_layer;
    color::Role selected_container;
    color::Role selected_label;
    color::Role selected_leading_icon;
    color::Role selected_trailing_icon;
    color::Role selected_state_layer;
    TypeRole group_label;
    color::Role group_label_color;
    double group_label_min_height;
    double group_label_start_padding;
    double group_label_end_padding;
    double divider_thickness;
    color::Role divider_color;
    double divider_horizontal_padding;
    double divider_vertical_padding;
    double window_vertical_margin;
    double window_horizontal_margin;
    double closed_scale;
};

inline constexpr GroupedMenuTokens grouped_menu_tokens{
    .group_container = color::Role::surface_container_low,
    .elevation = 2,
    .standalone_group = all_corners(CornerSize::large),
    .leading_group = {CornerSize::large, CornerSize::large, CornerSize::small, CornerSize::small},
    .middle_group = all_corners(CornerSize::small),
    .trailing_group = {CornerSize::small, CornerSize::small, CornerSize::large, CornerSize::large},
    .inactive_group = CornerSize::small,
    .group_gap = 2.0,
    .group_vertical_padding = 2.0,
    .item_horizontal_inset = 4.0,
    .item_min_height = 44.0,
    .item_min_width = 112.0,
    .item_max_width = 280.0,
    .item_horizontal_padding = 12.0,
    .item_vertical_padding = 12.0,
    .leading_icon_size = 20.0,
    .trailing_icon_size = 20.0,
    .icon_text_space = 8.0,
    .standalone_item = all_corners(CornerSize::extra_small),
    .leading_item = {CornerSize::medium, CornerSize::medium, CornerSize::extra_small, CornerSize::extra_small},
    .middle_item = all_corners(CornerSize::extra_small),
    .trailing_item = {CornerSize::extra_small, CornerSize::extra_small, CornerSize::medium, CornerSize::medium},
    .selected_item = CornerSize::medium,
    .label = TypeRole::label_large,
    .label_color = color::Role::on_surface,
    .leading_icon = color::Role::on_surface_variant,
    .trailing_icon = color::Role::on_surface_variant,
    .state_layer = color::Role::on_surface,
    .selected_container = color::Role::tertiary_container,
    .selected_label = color::Role::on_tertiary_container,
    .selected_leading_icon = color::Role::on_tertiary_container,
    .selected_trailing_icon = color::Role::on_tertiary_container,
    .selected_state_layer = color::Role::on_tertiary_container,
    .group_label = TypeRole::label_large,
    .group_label_color = color::Role::on_surface,
    .group_label_min_height = 32.0,
    .group_label_start_padding = 12.0,
    .group_label_end_padding = 4.0,
    .divider_thickness = 1.0,
    .divider_color = color::Role::outline_variant,
    .divider_horizontal_padding = 12.0,
    .divider_vertical_padding = 2.0,
    .window_vertical_margin = 48.0,
    .window_horizontal_margin = 8.0,
    .closed_scale = 0.8,
};

}
