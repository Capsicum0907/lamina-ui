#pragma once

#include <optional>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"

namespace lamina::widgets {

inline color::Argb mix_optional(const color::ColorScheme& scheme, std::optional<color::Role> from,
                                std::optional<color::Role> to, double t) {
    if (!from && !to) {
        return 0;
    }
    const color::Argb a = from ? scheme[*from] : color::with_alpha(scheme[*to], 0.0);
    const color::Argb b = to ? scheme[*to] : color::with_alpha(scheme[*from], 0.0);
    return color::mix(a, b, t);
}

}
