#pragma once

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/scrim.hpp"

namespace lamina::widgets {

inline void paint_scrim(render::Canvas& canvas, const render::Rect& area, const color::ColorScheme& scheme, double alpha) {
    const double a = std::clamp(alpha, 0.0, 1.0);
    if (a <= 0.0) {
        return;
    }
    const tokens::ScrimTokens& t = current_style().scrim;
    canvas.fill_rect(area, color::with_alpha(scheme[t.color], t.opacity * a));
}

}
