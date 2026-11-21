#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::tokens {

struct SplitPaneTokens {
    double divider_thickness;
    color::Role divider;
    double hit_thickness;
    double keyboard_step;
    double page_step;
    motion::SpringSpec handle_fade;
};

inline constexpr SplitPaneTokens split_pane_tokens{
    .divider_thickness = divider_tokens.thickness,
    .divider = divider_tokens.color,
    .hit_thickness = 8.0,
    .keyboard_step = 16.0,
    .page_step = 96.0,
    .handle_fade = motion_scheme.fast_effects(),
};

}
