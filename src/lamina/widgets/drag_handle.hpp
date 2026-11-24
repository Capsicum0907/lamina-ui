#pragma once

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/path.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/drag_handle.hpp"

namespace lamina::widgets {

class DragHandlePainter {
public:
    explicit DragHandlePainter(layout::Axis axis = layout::Axis::horizontal)
        : axis_(axis),
          width_(current_style().drag_handle.width, current_style().drag_handle.size_spring),
          height_(current_style().drag_handle.height, current_style().drag_handle.size_spring),
          active_(0.0, current_style().drag_handle.color_spring) {}

    void set_state(bool pressed, bool dragged) {
        const tokens::DragHandleTokens& t = current_style().drag_handle;
        const bool active = pressed || dragged;
        width_.animate_to(dragged ? t.dragged_width : pressed ? t.pressed_width : t.width);
        height_.animate_to(dragged ? t.dragged_height : pressed ? t.pressed_height : t.height);
        active_.animate_to(active ? 1.0 : 0.0);
    }

    void step(double dt) {
        width_.step(dt);
        height_.step(dt);
        active_.step(dt);
    }
    bool settled() const { return width_.settled() && height_.settled() && active_.settled(); }

    render::Rect rect(render::Point centre) const {
        const double across = width_.value();
        const double along = height_.value();
        if (axis_ == layout::Axis::horizontal) {
            return {centre.x - across / 2.0, centre.y - along / 2.0, across, along};
        }
        return {centre.x - along / 2.0, centre.y - across / 2.0, along, across};
    }

    double radius(const render::Rect& r) const {
        const tokens::DragHandleTokens& t = current_style().drag_handle;
        const double a = std::clamp(active_.value(), 0.0, 1.0);
        const double rest = tokens::corner_radius(t.shape, r.width, r.height);
        const double pressed = tokens::corner_radius(t.pressed_shape, r.width, r.height);
        return std::min(rest + (pressed - rest) * a, std::min(r.width, r.height) / 2.0);
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, render::Point centre, double opacity) const {
        if (opacity <= 0.0) {
            return;
        }
        const tokens::DragHandleTokens& t = current_style().drag_handle;
        const render::Rect r = rect(centre);
        const color::Argb c = color::mix(scheme[t.color], scheme[t.pressed_color], std::clamp(active_.value(), 0.0, 1.0));
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius(r))), color::scale_alpha(c, opacity));
    }

private:
    layout::Axis axis_;
    motion::Animated width_;
    motion::Animated height_;
    motion::Animated active_;
};

}
