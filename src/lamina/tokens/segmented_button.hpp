#pragma once

#include "lamina/builtin_resources.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SegmentedButtonTokens {
    double height;
    double min_width;
    double horizontal_padding;
    double vertical_padding;
    double outline_width;
    CornerSize corner;
    TypeRole label;
    double icon_size;
    double icon_spacing;
    icons::Symbol checkmark;
    color::Role active_container;
    color::Role active_content;
    color::Role inactive_content;
    color::Role outline;
    StateColor disabled_content;
    StateColor disabled_outline;
    color::Role disabled_active_container;
    int checked_z;
};

inline constexpr SegmentedButtonTokens segmented_button_tokens{
    .height = 40.0,
    .min_width = 58.0,
    .horizontal_padding = 12.0,
    .vertical_padding = 8.0,
    .outline_width = 1.0,
    .corner = CornerSize::full,
    .label = TypeRole::label_large,
    .icon_size = 18.0,
    .icon_spacing = 8.0,
    .checkmark = builtin::icons::check,
    .active_container = color::Role::secondary_container,
    .active_content = color::Role::on_secondary_container,
    .inactive_content = color::Role::on_surface,
    .outline = color::Role::outline,
    .disabled_content = {color::Role::on_surface, disabled_alpha},
    .disabled_outline = {color::Role::outline, 0.12},
    .disabled_active_container = color::Role::secondary_container,
    .checked_z = 5,
};

struct SegmentedButtonMotion {
    motion::SpringSpec label_shift;
    motion::SpringSpec check_scale;
    motion::SpringSpec check_fade;
    motion::SpringSpec icon_crossfade;
    motion::SpringSpec colors;
    motion::SpringSpec state_layer;
};

inline constexpr SegmentedButtonMotion segmented_button_motion{
    .label_shift = motion_scheme.fast_spatial(),
    .check_scale = motion_scheme.fast_spatial(),
    .check_fade = motion_scheme.default_effects(),
    .icon_crossfade = motion_scheme.default_effects(),
    .colors = motion_scheme.default_effects(),
    .state_layer = motion_scheme.fast_effects(),
};

}
