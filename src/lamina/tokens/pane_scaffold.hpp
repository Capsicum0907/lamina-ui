#pragma once

#include <array>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"

namespace lamina::tokens {

struct PaneScaffoldTokens {
    double partition_spacer;
    double preferred_width;
    double preferred_width_extra_large;
    double preferred_height;
    std::array<int, 3> priorities;
    motion::SpringSpec motion;
    double delayed_ratio;
    motion::SpringSpec anchoring;
    double anchoring_velocity_threshold;
    double handle_touch_target;
    double keyboard_step;
    double settle_distance;
};

inline constexpr PaneScaffoldTokens pane_scaffold_tokens{
    .partition_spacer = 24.0,
    .preferred_width = 360.0,
    .preferred_width_extra_large = 412.0,
    .preferred_height = 420.0,
    .priorities = {10, 5, 1},
    .motion = {0.8, 380.0},
    .delayed_ratio = 0.1,
    .anchoring = {0.8, 380.0},
    .anchoring_velocity_threshold = 200.0,
    .handle_touch_target = 48.0,
    .keyboard_step = 16.0,
    .settle_distance = 0.5,
};

}
