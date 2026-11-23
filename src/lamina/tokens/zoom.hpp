#pragma once

#include "lamina/motion/spring.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::tokens {

inline constexpr double millimetres_per_inch = 25.4;
inline constexpr double minimum_scaling_span_millimetres = 27.0;

struct ZoomTokens {
    double min_scale;
    double max_scale;
    double double_tap_scale;
    double zoomed_epsilon;
    double touch_slop;
    double span_slop;
    double min_span;
    double double_tap_timeout_seconds;
    double double_tap_min_seconds;
    double double_tap_slop;
    double double_tap_touch_slop;
    double wheel_zoom_factor;
    double pan_band_coefficient;
    double pan_band_extent;
    double scale_band_coefficient;
    double scale_band_ratio;
    motion::VelocityTrackerSpec tracking;
    double maximum_velocity;
    double minimum_fling_velocity;
    double edge_tolerance;
    motion::SpringSpec settle_spring;
    motion::SettleThreshold settle_threshold;
};

inline constexpr ZoomTokens zoom{
    .min_scale = 1.0,
    .max_scale = 5.0,
    .double_tap_scale = 2.5,
    .zoomed_epsilon = 0.001,
    .touch_slop = touch_slop,
    .span_slop = touch_slop * 2.0,
    .min_span = minimum_scaling_span_millimetres * fling_decay.dp_per_inch / millimetres_per_inch,
    .double_tap_timeout_seconds = 0.300,
    .double_tap_min_seconds = 0.040,
    .double_tap_slop = 100.0,
    .double_tap_touch_slop = touch_slop,
    .wheel_zoom_factor = 1.25,
    .pan_band_coefficient = 0.55,
    .pan_band_extent = 0.5,
    .scale_band_coefficient = 0.55,
    .scale_band_ratio = 1.5,
    .tracking = velocity_tracking,
    .maximum_velocity = maximum_fling_velocity,
    .minimum_fling_velocity = touch_scroll.minimum_fling_velocity,
    .edge_tolerance = touch_scroll.edge_tolerance,
    .settle_spring = motion_scheme.default_effects(),
    .settle_threshold = {1e-4, 1e-3},
};

}
