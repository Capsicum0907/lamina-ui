#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

struct DividerTokens {
    double thickness;
    double inset_start;
    double inset_end;
    color::Role color;
};

inline constexpr DividerTokens divider_tokens{
    .thickness = 1.0,
    .inset_start = 16.0,
    .inset_end = 16.0,
    .color = color::Role::outline_variant,
};

}
