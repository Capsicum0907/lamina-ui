#pragma once

#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/spline_decay.hpp"
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

inline constexpr motion::SplineDecaySpec fling_decay{
    .friction = 0.015,
    .inflection = 0.35,
    .start_tension = 0.5,
    .end_tension = 1.0,
    .samples = 100,
    .bisection_tolerance = 1e-5,
    .gravity = 9.80665,
    .inches_per_meter = 39.37,
    .dp_per_inch = 160.0,
    .physical_friction = 0.84,
    .deceleration_rate_base = 0.78,
    .deceleration_rate_reference = 0.9,
};

struct TouchScrollTokens {
    double slop;
    motion::VelocityTrackerSpec tracking;
    double maximum_velocity;
    double minimum_fling_velocity;
    double edge_tolerance;
    double horizontal_angle_limit_degrees;
    double vertical_angle_limit_degrees;
};

inline constexpr TouchScrollTokens touch_scroll{
    .slop = touch_slop,
    .tracking = velocity_tracking,
    .maximum_velocity = maximum_fling_velocity,
    .minimum_fling_velocity = 1.0,
    .edge_tolerance = 0.5,
    .horizontal_angle_limit_degrees = 30.0,
    .vertical_angle_limit_degrees = 90.0,
};

struct GestureTokens {
    motion::VelocityTrackerSpec tracking = velocity_tracking;
    double touch_slop = tokens::touch_slop;
    double maximum_fling_velocity = tokens::maximum_fling_velocity;
    double anchored_min_fling_velocity = tokens::anchored_min_fling_velocity;
    motion::DragGesture drag = drag_gesture;
    motion::SplineDecaySpec fling_decay = tokens::fling_decay;
    TouchScrollTokens touch_scroll = tokens::touch_scroll;
};

}
