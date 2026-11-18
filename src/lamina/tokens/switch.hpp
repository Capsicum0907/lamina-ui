#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

struct SwitchStateColors {
    color::Role track;
    color::Role thumb;
    color::Role icon;
    std::optional<color::Role> border;
};

struct SwitchTokens {
    double track_width;
    double track_height;
    double track_outline_width;
    double unchecked_thumb;
    double checked_thumb;
    double pressed_thumb;
    double icon_size;
    double state_layer_size;
    color::Role state_layer;
    SwitchStateColors checked;
    SwitchStateColors unchecked;
};

inline constexpr SwitchTokens switch_tokens{
    .track_width = 52.0,
    .track_height = 32.0,
    .track_outline_width = 2.0,
    .unchecked_thumb = 16.0,
    .checked_thumb = 24.0,
    .pressed_thumb = 28.0,
    .icon_size = 16.0,
    .state_layer_size = 40.0,
    .state_layer = color::Role::on_surface,
    .checked = {color::Role::primary, color::Role::on_primary, color::Role::on_primary_container, std::nullopt},
    .unchecked = {color::Role::surface_container_highest, color::Role::outline, color::Role::surface_container_highest,
                  color::Role::outline},
};

}
