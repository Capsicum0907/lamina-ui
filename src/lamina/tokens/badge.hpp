#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct BadgeTokens {
    double small_size;
    double large_size;
    double large_horizontal_padding;
    double small_offset;
    double large_horizontal_offset;
    double large_vertical_offset;
    TypeRole label;
    color::Role container;
    color::Role content;
    int max_count;
    const char* overflow_suffix;
};

inline constexpr BadgeTokens badge_tokens{
    .small_size = 6.0,
    .large_size = 16.0,
    .large_horizontal_padding = 4.0,
    .small_offset = 6.0,
    .large_horizontal_offset = 12.0,
    .large_vertical_offset = 14.0,
    .label = TypeRole::label_small,
    .container = color::Role::error,
    .content = color::Role::on_error,
    .max_count = 999,
    .overflow_suffix = "+",
};

}
