#pragma once

#include <cmath>
#include <memory>

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

template <Sized W>
layout::NodePtr centered_node(W& w) {
    return std::make_shared<layout::Leaf>([&w](const layout::Constraints&) { return layout::Size{w.width(), w.height()}; },
                                          [&w](const render::Rect& r) { w.set_origin({r.x, r.y + (r.height - w.height()) / 2.0}); });
}

template <WidthSettable W>
layout::NodePtr full_width_node(W& w) {
    return std::make_shared<layout::Leaf>(
        [&w](const layout::Constraints& c) { return layout::Size{std::isfinite(c.max_width) ? c.max_width : w.width(), w.height()}; },
        [&w](const render::Rect& r) {
            w.set_width(r.width);
            w.set_origin({r.x, r.y + (r.height - w.height()) / 2.0});
        });
}

template <SizeSettable W>
layout::NodePtr filled_node(W& w) {
    return std::make_shared<layout::Leaf>(
        [&w](const layout::Constraints& c) {
            return layout::Size{std::isfinite(c.max_width) ? c.max_width : w.width(), std::isfinite(c.max_height) ? c.max_height : w.height()};
        },
        [&w](const render::Rect& r) {
            w.set_size(r.width, r.height);
            w.set_origin({r.x, r.y});
        });
}

template <Measurable W>
layout::NodePtr measured_node(W& w) {
    return std::make_shared<layout::Leaf>([&w](const layout::Constraints& c) { return layout::Size(w.measure(c)); },
                                          [&w](const render::Rect& r) { w.arrange(r); });
}

template <Placeable W>
layout::NodePtr node(W& w) {
    if constexpr (Measurable<W>) {
        return measured_node(w);
    } else if constexpr (WidthSettable<W>) {
        return full_width_node(w);
    } else {
        return centered_node(w);
    }
}

}
