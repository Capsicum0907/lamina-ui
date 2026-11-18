#pragma once

#include <algorithm>
#include <cmath>

#include "lamina/color/scheme.hpp"

namespace lamina::color {

constexpr Argb with_alpha(Argb c, double alpha) {
    const double a = std::clamp(alpha, 0.0, 1.0);
    return (static_cast<Argb>(a * 255.0 + 0.5) << 24) | (c & 0x00ffffff);
}

inline Argb mix(Argb from, Argb to, double t) {
    const double k = std::clamp(t, 0.0, 1.0);
    Argb out = 0;
    for (int shift : {0, 8, 16, 24}) {
        const double a = static_cast<double>((from >> shift) & 0xff);
        const double b = static_cast<double>((to >> shift) & 0xff);
        out |= static_cast<Argb>(std::lround(a + (b - a) * k)) << shift;
    }
    return out;
}

inline ColorScheme mix(const ColorScheme& from, const ColorScheme& to, double t) {
    ColorScheme out{};
    for (std::size_t i = 0; i < role_count; ++i) {
        out.colors[i] = mix(from.colors[i], to.colors[i], t);
    }
    return out;
}

inline Argb overlay(Argb base, Argb layer, double opacity) {
    return mix(base, (base & 0xff000000) | (layer & 0x00ffffff), opacity);
}

}
