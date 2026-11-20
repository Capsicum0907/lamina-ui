#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::tokens {

struct RadioButtonTokens {
    double icon_size;
    double padding;
    double dot_size;
    double stroke_width;
    double state_layer_size;
    color::Role state_layer;
    color::Role selected;
    color::Role unselected;
    StateColor disabled_selected;
    StateColor disabled_unselected;
};

inline constexpr RadioButtonTokens radio_button_tokens{
    .icon_size = 20.0,
    .padding = 2.0,
    .dot_size = 12.0,
    .stroke_width = 2.0,
    .state_layer_size = 40.0,
    .state_layer = color::Role::on_surface,
    .selected = color::Role::primary,
    .unselected = color::Role::on_surface_variant,
    .disabled_selected = {color::Role::on_surface, 0.38},
    .disabled_unselected = {color::Role::on_surface, 0.38},
};

}
