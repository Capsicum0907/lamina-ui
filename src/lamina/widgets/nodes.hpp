#pragma once

#include <cmath>

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

inline layout::NodePtr node(Button& b) {
    return std::make_shared<layout::Leaf>([&b](const layout::Constraints&) { return layout::Size{b.width(), b.height()}; },
                                          [&b](const render::Rect& r) { b.set_origin({r.x, r.y + (r.height - b.height()) / 2.0}); });
}

template <typename W>
layout::NodePtr centered_node(W& w) {
    return std::make_shared<layout::Leaf>([&w](const layout::Constraints&) { return layout::Size{w.width(), w.height()}; },
                                          [&w](const render::Rect& r) { w.set_origin({r.x, r.y + (r.height - w.height()) / 2.0}); });
}

inline layout::NodePtr node(IconButton& b) { return centered_node(b); }
inline layout::NodePtr node(Fab& f) { return centered_node(f); }
inline layout::NodePtr node(Switch& s) { return centered_node(s); }
inline layout::NodePtr node(Checkbox& c) { return centered_node(c); }
inline layout::NodePtr node(RadioButton& r) { return centered_node(r); }

inline layout::NodePtr node(TextField& f) {
    return std::make_shared<layout::Leaf>([&f](const layout::Constraints&) { return layout::Size{f.bounds().width, f.bounds().height}; },
                                          [&f](const render::Rect& r) { f.set_origin({r.x, r.y}); });
}

inline layout::NodePtr node(Label& l) {
    return std::make_shared<layout::Leaf>([&l](const layout::Constraints& c) { return l.measure(c); },
                                          [&l](const render::Rect& r) { l.set_origin({r.x, r.y}); });
}

template <typename W>
layout::NodePtr full_width_node(W& w) {
    return std::make_shared<layout::Leaf>(
        [&w](const layout::Constraints& c) { return layout::Size{std::isfinite(c.max_width) ? c.max_width : w.width(), w.height()}; },
        [&w](const render::Rect& r) {
            w.set_width(r.width);
            w.set_origin({r.x, r.y + (r.height - w.height()) / 2.0});
        });
}

inline layout::NodePtr node(Slider& s) { return full_width_node(s); }
inline layout::NodePtr node(TabRow& t) { return full_width_node(t); }
inline layout::NodePtr node(NavigationBar& b) { return full_width_node(b); }

}
