#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct ListTokens {
    double leading_space;
    double trailing_space;
    double top_space;
    double bottom_space;
    double between_space;
    double one_line_height;
    double two_line_height;
    double three_line_height;
    double top_align_breakpoint;
    TypeRole headline;
    TypeRole overline;
    TypeRole supporting;
    TypeRole trailing_text;
    TypeRole avatar_label;
    double icon_size;
    double avatar_size;
    double image_size;
    CornerSize image_corner;
    double checkbox_slot;
    double radio_slot;
    double switch_width;
    double switch_height;
    color::Role container;
    color::Role selected_container;
    color::Role headline_color;
    color::Role content_color;
    color::Role selected_content;
    color::Role avatar_container;
    color::Role avatar_content;
    StateColor disabled_content;
    CornerSize corner;
    CornerSize hovered_corner;
    CornerSize focused_corner;
    CornerSize pressed_corner;
    CornerSize selected_corner;
    motion::SpringSpec color_spring;
    motion::SpringSpec shape_spring;
    motion::SpringSpec state_layer_spring;
    std::size_t overscan_rows;
};

inline constexpr ListTokens list_tokens{
    .leading_space = 16.0,
    .trailing_space = 16.0,
    .top_space = 10.0,
    .bottom_space = 10.0,
    .between_space = 12.0,
    .one_line_height = 56.0,
    .two_line_height = 72.0,
    .three_line_height = 88.0,
    .top_align_breakpoint = (88.0 + 72.0) / 2.0 - 10.0 - 10.0,
    .headline = TypeRole::body_large,
    .overline = TypeRole::label_small,
    .supporting = TypeRole::body_medium,
    .trailing_text = TypeRole::label_small,
    .avatar_label = TypeRole::title_medium,
    .icon_size = 24.0,
    .avatar_size = 40.0,
    .image_size = 56.0,
    .image_corner = CornerSize::small,
    .checkbox_slot = 24.0,
    .radio_slot = 24.0,
    .switch_width = 52.0,
    .switch_height = 32.0,
    .container = color::Role::surface,
    .selected_container = color::Role::secondary_container,
    .headline_color = color::Role::on_surface,
    .content_color = color::Role::on_surface_variant,
    .selected_content = color::Role::on_secondary_container,
    .avatar_container = color::Role::primary_container,
    .avatar_content = color::Role::on_primary_container,
    .disabled_content = {color::Role::on_surface, disabled_alpha},
    .corner = CornerSize::extra_small,
    .hovered_corner = CornerSize::medium,
    .focused_corner = CornerSize::large,
    .pressed_corner = CornerSize::large,
    .selected_corner = CornerSize::large,
    .color_spring = motion_scheme.default_effects(),
    .shape_spring = motion_scheme.fast_spatial(),
    .state_layer_spring = motion_scheme.fast_effects(),
    .overscan_rows = 2,
};

}
