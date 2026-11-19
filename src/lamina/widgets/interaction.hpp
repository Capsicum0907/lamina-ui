#pragma once

#include <algorithm>

#include "lamina/input/events.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::widgets {

constexpr bool contains(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

constexpr render::Rect touch_target(const render::Rect& r) {
    const double w = std::max(r.width, tokens::minimum_interactive_size);
    const double h = std::max(r.height, tokens::minimum_interactive_size);
    return {r.x - (w - r.width) / 2.0, r.y - (h - r.height) / 2.0, w, h};
}

struct PressState {
    bool hovered = false;
    bool pressed = false;

    bool update(const input::PointerEvent& e, const render::Rect& hit) {
        const bool over = e.action != input::PointerAction::leave && contains(hit, e.x_dp, e.y_dp);
        bool clicked = false;
        hovered = over;
        switch (e.action) {
        case input::PointerAction::down:
            pressed = over;
            break;
        case input::PointerAction::up:
            clicked = pressed && over;
            pressed = false;
            break;
        case input::PointerAction::leave:
            pressed = false;
            break;
        case input::PointerAction::move:
            break;
        }
        return clicked;
    }

    double state_layer_opacity() const {
        if (pressed) {
            return tokens::state_layer_opacity::pressed;
        }
        return hovered ? tokens::state_layer_opacity::hover : 0.0;
    }
};

inline render::Path ring_path(const render::Rect& r, double radius, double width) {
    render::Path p = render::rounded_rect_path(r, render::uniform_radii(radius));
    const render::Rect inner{r.x + width, r.y + width, r.width - 2 * width, r.height - 2 * width};
    p.append(render::rounded_rect_path(inner, render::uniform_radii(std::max(0.0, radius - width))));
    return p;
}

inline render::Rect circle_rect(render::Point center, double radius) {
    return {center.x - radius, center.y - radius, 2 * radius, 2 * radius};
}

inline render::Path circle_path(render::Point center, double radius) {
    return render::rounded_rect_path(circle_rect(center, radius), render::uniform_radii(radius));
}

}
