#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::tokens {

enum class CheckboxState { off, on, indeterminate };

struct UnitPoint {
    double x;
    double y;
};

struct CheckboxTokens {
    double box_size;
    double padding;
    double corner_radius;
    double stroke_width;
    double state_layer_size;
    UnitPoint check_left;
    UnitPoint check_cross;
    UnitPoint check_right;
    double indeterminate_center;
    double uncheck_snap_delay_seconds;
    color::Role state_layer;
    color::Role checked_box;
    color::Role checkmark;
    color::Role unchecked_border;
    StateColor disabled_checked_box;
    StateColor disabled_checkmark;
    StateColor disabled_unchecked_border;
};

inline constexpr CheckboxTokens checkbox_tokens{
    .box_size = 20.0,
    .padding = 2.0,
    .corner_radius = 2.0,
    .stroke_width = 2.0,
    .state_layer_size = 40.0,
    .check_left = {0.2, 0.5},
    .check_cross = {0.4, 0.7},
    .check_right = {0.8, 0.3},
    .indeterminate_center = 0.5,
    .uncheck_snap_delay_seconds = 0.1,
    .state_layer = color::Role::on_surface,
    .checked_box = color::Role::primary,
    .checkmark = color::Role::on_primary,
    .unchecked_border = color::Role::on_surface_variant,
    .disabled_checked_box = {color::Role::on_surface, 0.38},
    .disabled_checkmark = {color::Role::surface},
    .disabled_unchecked_border = {color::Role::on_surface, 0.38},
};

}
