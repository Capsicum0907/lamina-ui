#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina::tokens {

struct ScrimTokens {
    color::Role color;
    double opacity;
};

inline constexpr ScrimTokens scrim_tokens{.color = color::Role::scrim, .opacity = 0.32};

}
