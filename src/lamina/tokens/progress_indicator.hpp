#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

struct CircularProgressTokens {
    double size;
    double wavy_size;
    double active_thickness;
    double track_thickness;
    double track_active_space;
    double wave_amplitude;
    double wavelength;
    double waves_per_second;
    double wave_start_progress;
    double wave_end_progress;
    double sample_length;
    color::Role indicator;
    color::Role track;
};

inline constexpr CircularProgressTokens circular_progress_tokens{
    .size = 40.0,
    .wavy_size = 48.0,
    .active_thickness = 4.0,
    .track_thickness = 4.0,
    .track_active_space = 4.0,
    .wave_amplitude = 1.6,
    .wavelength = 15.0,
    .waves_per_second = 1.0,
    .wave_start_progress = 0.1,
    .wave_end_progress = 0.95,
    .sample_length = 1.0,
    .indicator = color::Role::primary,
    .track = color::Role::secondary_container,
};

}
