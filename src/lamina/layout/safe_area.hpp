#pragma once

#include <algorithm>

#include "lamina/layout/layout.hpp"
#include "lamina/render/geometry.hpp"

namespace lamina::layout {

constexpr Insets insets_in_dp(const Insets& units, double dp_per_unit) {
    const auto edge = [dp_per_unit](double u) { return std::max(0.0, u) * dp_per_unit; };
    return {edge(units.left), edge(units.top), edge(units.right), edge(units.bottom)};
}

constexpr Insets safe_area_insets(const render::Rect& window, const render::Rect& safe, double dp_per_unit) {
    return insets_in_dp({safe.x - window.x, safe.y - window.y, window.x + window.width - safe.x - safe.width,
                         window.y + window.height - safe.y - safe.height},
                        dp_per_unit);
}

constexpr render::Rect inset_rect(const render::Rect& rect, const Insets& insets) {
    return {rect.x + insets.left, rect.y + insets.top, std::max(0.0, rect.width - insets.left - insets.right),
            std::max(0.0, rect.height - insets.top - insets.bottom)};
}

}
