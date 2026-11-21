#pragma once

#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/velocity_tracker.hpp"

namespace lamina::tokens {

inline constexpr motion::VelocityTrackerSpec velocity_tracking{
    .history = 20,
    .horizon_seconds = 0.100,
    .stopped_seconds = 0.040,
    .min_samples = 2,
    .degree = 2,
};

inline constexpr double touch_slop = 8.0;
inline constexpr double maximum_fling_velocity = 8000.0;
inline constexpr double anchored_min_fling_velocity = 125.0;

inline constexpr motion::DragGesture drag_gesture{touch_slop, velocity_tracking, maximum_fling_velocity};

}
