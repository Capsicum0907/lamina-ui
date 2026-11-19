#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

struct FocusIndicatorTokens {
    double thickness;
    double outer_offset;
    color::Role color;
};

inline constexpr FocusIndicatorTokens focus_indicator{
    .thickness = 3.0,
    .outer_offset = 2.0,
    .color = color::Role::secondary,
};

}
