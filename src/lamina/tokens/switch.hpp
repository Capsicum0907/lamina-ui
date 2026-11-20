#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::tokens {

struct SwitchStateColors {
    color::Role track;
    color::Role thumb;
    color::Role icon;
    std::optional<color::Role> border;
};

struct SwitchDisabledColors {
    StateColor track;
    StateColor thumb;
    StateColor icon;
    std::optional<StateColor> border;
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
    SwitchDisabledColors disabled_checked;
    SwitchDisabledColors disabled_unchecked;
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
    .disabled_checked = {{color::Role::on_surface, 0.12, true},
                         {color::Role::surface, 1.0, true},
                         {color::Role::on_surface, 0.38, true},
                         std::nullopt},
    .disabled_unchecked = {{color::Role::surface_container_highest, 0.12, true},
                           {color::Role::on_surface, 0.38, true},
                           {color::Role::surface_container_highest, 0.38, true},
                           StateColor{color::Role::on_surface, 0.12, true}},
};

}
