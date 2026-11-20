#pragma once

#include <algorithm>
#include <optional>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

inline color::Argb resolve(const color::ColorScheme& scheme, const tokens::StateColor& c) {
    const color::Argb argb = color::scale_alpha(scheme[c.role], c.opacity);
    if (c.over_surface) {
        return color::source_over(argb, scheme[color::Role::surface]);
    }
    return c.over ? color::source_over(argb, scheme[*c.over]) : argb;
}

inline color::Argb resolve(const color::ColorScheme& scheme, const std::optional<tokens::StateColor>& c) {
    return c ? resolve(scheme, *c) : 0;
}

inline color::Argb mix_resolved(const color::ColorScheme& scheme, const std::optional<tokens::StateColor>& from,
                                const std::optional<tokens::StateColor>& to, double t) {
    if (!from && !to) {
        return 0;
    }
    const color::Argb a = from ? resolve(scheme, *from) : color::with_alpha(resolve(scheme, *to), 0.0);
    const color::Argb b = to ? resolve(scheme, *to) : color::with_alpha(resolve(scheme, *from), 0.0);
    return color::mix(a, b, t);
}

class Enablement {
public:
    explicit Enablement(bool enabled = true)
        : enabled_(enabled), amount_(enabled ? 1.0 : 0.0, tokens::motion_scheme.default_effects()) {}

    bool enabled() const { return enabled_; }
    bool set(bool enabled) {
        if (enabled == enabled_) {
            return false;
        }
        enabled_ = enabled;
        amount_.animate_to(enabled ? 1.0 : 0.0);
        return true;
    }

    void step(double dt_seconds) { amount_.step(dt_seconds); }
    bool settled() const { return amount_.settled(); }
    double amount() const { return std::clamp(amount_.value(), 0.0, 1.0); }

    color::Argb mix(color::Argb disabled, color::Argb enabled) const {
        const double a = amount();
        if (a >= 1.0) {
            return enabled;
        }
        if (a <= 0.0) {
            return disabled;
        }
        return color::mix(fill_transparent(disabled, enabled), fill_transparent(enabled, disabled), a);
    }

private:
    static color::Argb fill_transparent(color::Argb c, color::Argb other) {
        return color::alpha(c) == 0 ? color::with_alpha(other, 0.0) : c;
    }

    bool enabled_;
    motion::Animated amount_;
};

}
