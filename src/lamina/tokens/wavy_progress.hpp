#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/easing.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/shapes/corner_rounding.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::tokens {

struct WavyProgressTokens {
    color::Role indicator;
    color::Role track;
    double linear_width;
    double linear_height;
    double linear_thickness;
    double linear_track_thickness;
    double linear_gap;
    double linear_stop_size;
    double linear_wavelength;
    double linear_indeterminate_wavelength;
    double circular_size;
    double circular_thickness;
    double circular_track_thickness;
    double circular_gap;
    double circular_wavelength;
    double wave_start_progress;
    double wave_end_progress;
    int min_circular_vertices;
    float star_inner_radius;
    shapes::CornerRounding star_rounding;
    shapes::CornerRounding star_inner_rounding;
    double min_offset_seconds;
    int flatten_segments;
};

inline constexpr WavyProgressTokens wavy_progress_tokens{
    .indicator = color::Role::primary,
    .track = color::Role::secondary_container,
    .linear_width = 240.0,
    .linear_height = 10.0,
    .linear_thickness = 4.0,
    .linear_track_thickness = 4.0,
    .linear_gap = 4.0,
    .linear_stop_size = 4.0,
    .linear_wavelength = 40.0,
    .linear_indeterminate_wavelength = 20.0,
    .circular_size = 48.0,
    .circular_thickness = 4.0,
    .circular_track_thickness = 4.0,
    .circular_gap = 4.0,
    .circular_wavelength = 15.0,
    .wave_start_progress = 0.1,
    .wave_end_progress = 0.95,
    .min_circular_vertices = 5,
    .star_inner_radius = 0.75f,
    .star_rounding = {0.35f, 0.4f},
    .star_inner_rounding = {0.5f, 0.0f},
    .min_offset_seconds = 0.05,
    .flatten_segments = 12,
};

struct WavyProgressMotion {
    motion::TweenSpec progress;
    motion::TweenSpec amplitude_up;
    motion::TweenSpec amplitude_down;
    motion::TweenSpec circular_handoff_min;
    motion::TweenSpec circular_handoff_max;
};

inline constexpr WavyProgressMotion wavy_progress_motion{
    .progress = {duration_ms::long_[1] / 1000.0, easing::linear},
    .amplitude_up = {duration_ms::long_[1] / 1000.0, easing::standard},
    .amplitude_down = {duration_ms::long_[1] / 1000.0, easing::emphasized_accelerate},
    .circular_handoff_min = {duration_ms::medium[1] / 1000.0, easing::emphasized_decelerate},
    .circular_handoff_max = {duration_ms::long_[3] / 1000.0, easing::emphasized_decelerate},
};

struct KeyframeSegment {
    double delay_ms;
    double duration_ms;
};

struct LinearIndeterminateTokens {
    double cycle_ms;
    KeyframeSegment first_head;
    KeyframeSegment first_tail;
    KeyframeSegment second_head;
    KeyframeSegment second_tail;
    CubicBezier easing;
};

inline constexpr LinearIndeterminateTokens linear_indeterminate{
    .cycle_ms = 1750.0,
    .first_head = {0.0, 1000.0},
    .first_tail = {250.0, 1000.0},
    .second_head = {650.0, 850.0},
    .second_tail = {900.0, 850.0},
    .easing = easing::emphasized_accelerate,
};

struct CircularIndeterminateTokens {
    double cycle_ms;
    double min_progress;
    double max_progress;
    CubicBezier progress_easing;
    double turn_ms;
    double turn_every_ms;
    double global_degrees;
    double base_degrees;
    CubicBezier turn_easing;
};

inline constexpr CircularIndeterminateTokens circular_indeterminate{
    .cycle_ms = 6000.0,
    .min_progress = 0.1,
    .max_progress = 0.87,
    .progress_easing = easing::standard,
    .turn_ms = 300.0,
    .turn_every_ms = 1500.0,
    .global_degrees = 1080.0,
    .base_degrees = 90.0,
    .turn_easing = easing::emphasized_decelerate,
};

}
