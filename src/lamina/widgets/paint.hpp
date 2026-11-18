#pragma once

#include <cmath>
#include <optional>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"

namespace lamina::widgets {

inline color::Argb source_over(color::Argb top, color::Argb bottom) {
    const double at = (top >> 24) / 255.0;
    const double ab = (bottom >> 24) / 255.0;
    const double ao = at + ab * (1.0 - at);
    if (ao <= 0.0) {
        return 0;
    }
    color::Argb out = static_cast<color::Argb>(std::lround(ao * 255.0)) << 24;
    for (int shift : {0, 8, 16}) {
        const double ct = static_cast<double>((top >> shift) & 0xff);
        const double cb = static_cast<double>((bottom >> shift) & 0xff);
        out |= static_cast<color::Argb>(std::lround((ct * at + cb * ab * (1.0 - at)) / ao)) << shift;
    }
    return out;
}

inline color::Argb with_state_layer(color::Argb container, color::Argb content, double opacity) {
    return source_over(color::with_alpha(content, opacity), container);
}

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
