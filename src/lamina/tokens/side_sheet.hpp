#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SideSheetTokens {
    double width;
    double detached_margin;
    color::Role modal_container;
    color::Role standard_container;
    color::Role content;
    CornerSize corner;
    double modal_elevation;
    double standard_elevation;
    TypeRole headline_type;
    color::Role headline;
    color::Role divider;
    double header_height;
    double padding;
    double header_button_padding;
    double back_headline_space;
    double actions_padding;
    double action_space;
    ButtonSize action_size;
    IconButtonSize header_button_size;
    color::Role header_icon;
    double hide_fraction;
    double significant_velocity;
};

inline constexpr SideSheetTokens side_sheet_tokens{
    .width = 256.0,
    .detached_margin = 16.0,
    .modal_container = color::Role::surface_container_low,
    .standard_container = color::Role::surface,
    .content = color::Role::on_surface,
    .corner = CornerSize::large,
    .modal_elevation = elevation_dp[1],
    .standard_elevation = elevation_dp[0],
    .headline_type = TypeRole::title_large,
    .headline = color::Role::on_surface_variant,
    .divider = color::Role::outline,
    .header_height = 72.0,
    .padding = 24.0,
    .header_button_padding = 12.0,
    .back_headline_space = 4.0,
    .actions_padding = 16.0,
    .action_space = 8.0,
    .action_size = ButtonSize::small,
    .header_button_size = IconButtonSize::small,
    .header_icon = color::Role::on_surface_variant,
    .hide_fraction = 0.5,
    .significant_velocity = 500.0,
};

inline constexpr motion::TweenSpec side_sheet_motion{0.275, easing::emphasized};

}
