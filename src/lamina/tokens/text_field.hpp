#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct OutlinedTextFieldTokens {
    double height = 56.0;
    double min_width = 280.0;
    CornerSize corner = CornerSize::extra_small;
    double padding = 16.0;
    double outline_width = 1.0;
    double focus_outline_width = 2.0;
    double label_gap_padding = 4.0;
    TypeRole input_type = TypeRole::body_large;
    TypeRole label_type = TypeRole::body_large;
    TypeRole minimized_label_type = TypeRole::body_small;
    color::Role input = color::Role::on_surface;
    color::Role label = color::Role::on_surface_variant;
    color::Role focus_label = color::Role::primary;
    color::Role hover_label = color::Role::on_surface;
    color::Role outline = color::Role::outline;
    color::Role hover_outline = color::Role::on_surface;
    color::Role focus_outline = color::Role::primary;
    color::Role caret = color::Role::primary;
    double caret_width = 2.0;
    double caret_blink_seconds = 0.5;
    double composition_underline = 1.0;
    color::Role selection = color::Role::primary;
    double selection_opacity = 0.4;
    double vertical_padding = 16.0;
    std::size_t undo_depth = 100;
    double undo_pause_seconds = 1.0;
};

inline constexpr OutlinedTextFieldTokens outlined_text_field{};

}
