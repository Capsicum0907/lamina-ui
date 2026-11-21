#pragma once

#include <cstddef>

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct TableTokens {
    color::Role container;
    color::Role header_container;
    double row_height;
    double header_height;
    double cell_leading_padding;
    double cell_trailing_padding;
    TypeRole header_label;
    color::Role header_label_color;
    TypeRole cell_label;
    color::Role cell_label_color;
    double divider_thickness;
    color::Role divider;
    double header_divider_inset;
    color::Role selected_container;
    color::Role selected_label;
    color::Role state_layer;
    double sort_icon_size;
    double sort_icon_gap;
    color::Role sort_icon;
    color::Role active_sort_icon;
    icons::Symbol ascending_symbol;
    icons::Symbol descending_symbol;
    double resize_hit_width;
    double default_column_width;
    double default_min_column_width;
    std::size_t overscan_rows;
    double focus_ring_corner;
    StateColor disabled_label;
    motion::SpringSpec color_spring;
    motion::SpringSpec state_layer_spring;
};

inline constexpr TableTokens table_tokens{
    .container = color::Role::surface,
    .header_container = color::Role::surface_container,
    .row_height = 52.0,
    .header_height = 56.0,
    .cell_leading_padding = 16.0,
    .cell_trailing_padding = 16.0,
    .header_label = TypeRole::title_small,
    .header_label_color = color::Role::on_surface,
    .cell_label = TypeRole::body_medium,
    .cell_label_color = color::Role::on_surface,
    .divider_thickness = 1.0,
    .divider = color::Role::outline_variant,
    .header_divider_inset = 16.0,
    .selected_container = color::Role::secondary_container,
    .selected_label = color::Role::on_secondary_container,
    .state_layer = color::Role::on_surface,
    .sort_icon_size = 18.0,
    .sort_icon_gap = 4.0,
    .sort_icon = color::Role::on_surface_variant,
    .active_sort_icon = color::Role::on_surface,
    .ascending_symbol = builtin::icons::arrow_upward,
    .descending_symbol = builtin::icons::arrow_downward,
    .resize_hit_width = 8.0,
    .default_column_width = 160.0,
    .default_min_column_width = 56.0,
    .overscan_rows = 2,
    .focus_ring_corner = 0.0,
    .disabled_label = {color::Role::on_surface, 0.38},
    .color_spring = motion_scheme.default_effects(),
    .state_layer_spring = motion_scheme.fast_effects(),
};

}
