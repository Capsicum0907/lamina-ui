#pragma once

#include <cmath>
#include <vector>

#include "lamina/widgets/side_navigation.hpp"
#include "support.hpp"

namespace lamina::test {

inline constexpr double navigation_tolerance = 0.5;

inline bool same_rect(const render::Rect& a, const render::Rect& b) {
    const double e = navigation_tolerance;
    return std::abs(a.x - b.x) <= e && std::abs(a.y - b.y) <= e && std::abs(a.width - b.width) <= e && std::abs(a.height - b.height) <= e;
}

inline bool rect_inside(const render::Rect& inner, const render::Rect& outer) {
    const double e = navigation_tolerance;
    return inner.x >= outer.x - e && inner.y >= outer.y - e && inner.x + inner.width <= outer.x + outer.width + e &&
           inner.y + inner.height <= outer.y + outer.height + e;
}

struct PaintedItems {
    std::vector<int> icons;
    std::vector<RecordedFill> unknown;
};

inline PaintedItems painted_items(const std::vector<RecordedFill>& fills, const widgets::SideNavigation& s, const render::Rect& elsewhere) {
    PaintedItems out{std::vector<int>(s.count(), 0), {}};
    for (const RecordedFill& f : fills) {
        if (rect_inside(f.bounds, elsewhere) || same_rect(f.bounds, s.bounds())) {
            continue;
        }
        bool known = false;
        for (std::size_t i = 0; i < s.count() && !known; ++i) {
            if (rect_inside(f.bounds, s.icon_rect(i))) {
                ++out.icons[i];
                known = true;
            } else if (same_rect(f.bounds, s.indicator_rect(i)) || same_rect(f.bounds, s.shape_rect(i))) {
                known = true;
            } else {
                const render::Rect label = s.label_rect(i);
                const render::Rect row = s.item_rect(i);
                known = rect_inside(f.bounds, {label.x, row.y, label.width, row.height});
            }
        }
        if (!known) {
            out.unknown.push_back(f);
        }
    }
    return out;
}

}
