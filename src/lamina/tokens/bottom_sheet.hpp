#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct BottomSheetTokens {
    color::Role container;
    color::Role content;
    Corners shape;
    double max_width;
    double peek_height;
    double modal_shadow_elevation;
    double standard_shadow_elevation;
    double drag_handle_width;
    double drag_handle_height;
    CornerSize drag_handle_corner;
    color::Role drag_handle;
    double drag_handle_vertical_padding;
    color::Role drag_handle_state_layer;
    double positional_threshold;
    double velocity_threshold;
    double boundary_dampening_zone;
    double partial_screen_fraction;
    double partial_sheet_fraction;
};

inline constexpr BottomSheetTokens bottom_sheet_tokens{
    .container = color::Role::surface_container_low,
    .content = color::Role::on_surface,
    .shape = corners::extra_large_top,
    .max_width = 640.0,
    .peek_height = 56.0,
    .modal_shadow_elevation = elevation_dp[0],
    .standard_shadow_elevation = elevation_dp[1],
    .drag_handle_width = 32.0,
    .drag_handle_height = 4.0,
    .drag_handle_corner = CornerSize::extra_large,
    .drag_handle = color::Role::on_surface_variant,
    .drag_handle_vertical_padding = 22.0,
    .drag_handle_state_layer = color::Role::on_surface,
    .positional_threshold = 56.0,
    .velocity_threshold = anchored_min_fling_velocity,
    .boundary_dampening_zone = 125.0,
    .partial_screen_fraction = 0.5,
    .partial_sheet_fraction = 0.5,
};

struct SheetMotion {
    motion::SpringSpec show;
    motion::SpringSpec hide;
    motion::SpringSpec settle;
    motion::SpringSpec scrim;
};

inline constexpr SheetMotion bottom_sheet_motion{
    .show = motion_scheme.default_spatial(),
    .hide = motion_scheme.fast_effects(),
    .settle = motion_scheme.default_spatial(),
    .scrim = motion_scheme.default_effects(),
};

}
