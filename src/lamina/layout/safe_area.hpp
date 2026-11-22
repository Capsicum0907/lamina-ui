#pragma once

#include <algorithm>

#include "lamina/layout/layout.hpp"
#include "lamina/render/geometry.hpp"

namespace lamina::layout {

constexpr Insets safe_area_insets(const render::Rect& window, const render::Rect& safe, double dp_per_unit) {
    const auto edge = [dp_per_unit](double units) { return std::max(0.0, units) * dp_per_unit; };
    return {edge(safe.x - window.x), edge(safe.y - window.y), edge(window.x + window.width - safe.x - safe.width),
            edge(window.y + window.height - safe.y - safe.height)};
}

constexpr render::Rect inset_rect(const render::Rect& rect, const Insets& insets) {
    return {rect.x + insets.left, rect.y + insets.top, std::max(0.0, rect.width - insets.left - insets.right),
            std::max(0.0, rect.height - insets.top - insets.bottom)};
}

}
