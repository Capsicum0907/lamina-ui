#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct NavigationDrawerTokens {
    color::Role modal_container;
    color::Role standard_container;
    Corners modal_shape;
    double min_width;
    double max_width;
    double modal_elevation;
    double standard_elevation;
    double top_space;
    double item_height;
    CornerSize item_shape;
    double item_inset;
    double item_padding_start;
    double item_padding_end;
    double icon_size;
    double icon_label_space;
    double badge_space;
    TypeRole label_type;
    TypeRole badge_type;
    color::Role selected_container;
    color::Role selected_icon;
    color::Role selected_label;
    color::Role selected_badge;
    color::Role unselected_icon;
    color::Role unselected_label;
    color::Role unselected_badge;
    color::Role selected_state_layer;
    color::Role unselected_state_layer;
    StateColor disabled_content;
    TypeRole headline_type;
    color::Role headline;
    double headline_height;
    double headline_padding;
    double divider_inset;
    double divider_top_space;
    double divider_bottom_space;
    double positional_fraction;
    double velocity_threshold;
};

inline constexpr NavigationDrawerTokens navigation_drawer_tokens{
    .modal_container = color::Role::surface_container_low,
    .standard_container = color::Role::surface,
    .modal_shape = corners::large_end,
    .min_width = 240.0,
    .max_width = 360.0,
    .modal_elevation = elevation_dp[0],
    .standard_elevation = elevation_dp[0],
    .top_space = 12.0,
    .item_height = 56.0,
    .item_shape = CornerSize::full,
    .item_inset = 12.0,
    .item_padding_start = 16.0,
    .item_padding_end = 24.0,
    .icon_size = 24.0,
    .icon_label_space = 12.0,
    .badge_space = 12.0,
    .label_type = TypeRole::label_large,
    .badge_type = TypeRole::label_large,
    .selected_container = color::Role::secondary_container,
    .selected_icon = color::Role::on_secondary_container,
    .selected_label = color::Role::on_secondary_container,
    .selected_badge = color::Role::on_secondary_container,
    .unselected_icon = color::Role::on_surface_variant,
    .unselected_label = color::Role::on_surface_variant,
    .unselected_badge = color::Role::on_surface_variant,
    .selected_state_layer = color::Role::on_secondary_container,
    .unselected_state_layer = color::Role::on_surface,
    .disabled_content = {color::Role::on_surface, disabled_alpha},
    .headline_type = TypeRole::title_small,
    .headline = color::Role::on_surface_variant,
    .headline_height = 56.0,
    .headline_padding = 28.0,
    .divider_inset = 28.0,
    .divider_top_space = 8.0,
    .divider_bottom_space = 3.0,
    .positional_fraction = 0.5,
    .velocity_threshold = 400.0,
};

struct DrawerMotion {
    motion::SpringSpec open;
    motion::SpringSpec close;
    motion::TweenSpec settle;
    motion::SpringSpec colors;
};

inline constexpr DrawerMotion navigation_drawer_motion{
    .open = motion_scheme.default_spatial(),
    .close = motion_scheme.fast_effects(),
    .settle = {0.256, easing::legacy},
    .colors = motion_scheme.default_effects(),
};

}
