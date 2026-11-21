#pragma once

#include <array>
#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct ToggleFabTokens {
    std::array<double, static_cast<std::size_t>(FabSize::count)> initial_corner;
    double final_size;
    double final_icon_size;
    std::size_t elevation;
    color::Role container;
    color::Role checked_container;
    color::Role content;
    color::Role checked_content;
    double icon_swap_progress;
    motion::SpringSpec progress;
};

inline constexpr ToggleFabTokens toggle_fab_tokens{
    .initial_corner = {16.0, 20.0, 28.0},
    .final_size = 56.0,
    .final_icon_size = 20.0,
    .elevation = 3,
    .container = color::Role::primary_container,
    .checked_container = color::Role::primary,
    .content = color::Role::on_primary_container,
    .checked_content = color::Role::on_primary,
    .icon_swap_progress = 0.5,
    .progress = motion_scheme.fast_spatial(),
};

struct FabMenuTokens {
    double padding_horizontal;
    double button_padding_bottom;
    double close_button_between_space;
    double item_height;
    double item_min_width;
    double item_between_space;
    double item_leading_space;
    double item_trailing_space;
    double item_icon_label_space;
    double item_icon_size;
    CornerSize item_shape;
    TypeRole item_label;
    color::Role item_container;
    color::Role item_content;
    motion::SpringSpec stagger;
    double stagger_threshold;
    motion::SpringSpec item_width;
    motion::SpringSpec item_alpha;
};

inline constexpr FabMenuTokens fab_menu_tokens{
    .padding_horizontal = 16.0,
    .button_padding_bottom = 16.0,
    .close_button_between_space = 8.0,
    .item_height = 56.0,
    .item_min_width = 56.0,
    .item_between_space = 4.0,
    .item_leading_space = 24.0,
    .item_trailing_space = 24.0,
    .item_icon_label_space = 8.0,
    .item_icon_size = 24.0,
    .item_shape = CornerSize::full,
    .item_label = TypeRole::title_medium,
    .item_container = color::Role::primary_container,
    .item_content = color::Role::on_primary_container,
    .stagger = motion_scheme.slow_effects(),
    .stagger_threshold = 1.0,
    .item_width = motion_scheme.fast_spatial(),
    .item_alpha = motion_scheme.fast_effects(),
};

}
