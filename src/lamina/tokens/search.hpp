#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SearchTokens {
    color::Role container;
    double height;
    CornerSize collapsed_shape;
    CornerSize docked_shape;
    color::Role divider;
    color::Role leading_icon;
    color::Role trailing_icon;
    StateColor disabled_icon;
    double icon_size;
    double icon_slot;
    double icon_offset;
    double icon_padding;
    double padding;
    double min_width;
    double max_width;
    double vertical_padding;
    double docked_min_height;
    double docked_max_height_ratio;
    double tonal_elevation;
    double shadow_elevation;
    double compact_width;
    double expanded_threshold;
    IconButtonSize icon_button_size;
};

inline constexpr SearchTokens search_tokens{
    .container = color::Role::surface_container_high,
    .height = 56.0,
    .collapsed_shape = CornerSize::full,
    .docked_shape = CornerSize::extra_large,
    .divider = color::Role::outline,
    .leading_icon = color::Role::on_surface,
    .trailing_icon = color::Role::on_surface_variant,
    .disabled_icon = {color::Role::on_surface, disabled_alpha},
    .icon_size = 24.0,
    .icon_slot = 48.0,
    .icon_offset = 4.0,
    .icon_padding = 12.0,
    .padding = 16.0,
    .min_width = 360.0,
    .max_width = 720.0,
    .vertical_padding = 8.0,
    .docked_min_height = 240.0,
    .docked_max_height_ratio = 2.0 / 3.0,
    .tonal_elevation = elevation_dp[0],
    .shadow_elevation = elevation_dp[0],
    .compact_width = 600.0,
    .expanded_threshold = 0.02,
    .icon_button_size = IconButtonSize::small,
};

struct SearchMotion {
    motion::SpringSpec expand;
    motion::SpringSpec collapse;
};

inline constexpr SearchMotion search_motion{
    .expand = motion_scheme.slow_spatial(),
    .collapse = motion_scheme.default_spatial(),
};

}
