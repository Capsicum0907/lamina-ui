#pragma once

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

inline layout::NodePtr node(Button& b) {
    return std::make_shared<layout::Leaf>([&b](const layout::Constraints&) { return layout::Size{b.width(), b.height()}; },
                                          [&b](const render::Rect& r) { b.set_origin({r.x, r.y + (r.height - b.height()) / 2.0}); });
}

inline layout::NodePtr node(TextField& f) {
    return std::make_shared<layout::Leaf>([&f](const layout::Constraints&) { return layout::Size{f.bounds().width, f.bounds().height}; },
                                          [&f](const render::Rect& r) { f.set_origin({r.x, r.y}); });
}

inline layout::NodePtr node(Label& l) {
    return std::make_shared<layout::Leaf>([&l](const layout::Constraints& c) { return l.measure(c); },
                                          [&l](const render::Rect& r) { l.set_origin({r.x, r.y}); });
}

}
