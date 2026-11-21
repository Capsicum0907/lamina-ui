#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

#include "lamina/input/events.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::widgets {

constexpr bool contains(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

constexpr render::Rect touch_target(const render::Rect& r, double minimum = tokens::minimum_interactive_size) {
    const double w = std::max(r.width, minimum);
    const double h = std::max(r.height, minimum);
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

template <typename Items, typename HitRect>
std::optional<std::size_t> update_presses(Items& items, const input::PointerEvent& e, HitRect hit) {
    std::optional<std::size_t> clicked;
    for (std::size_t i = 0; i < items.size(); ++i) {
        auto& item = items[i];
        if (item.press.update(e, hit(i))) {
            clicked = i;
        }
        item.state_layer.animate_to(item.press.state_layer_opacity());
    }
    return clicked;
}

template <typename Items>
void cancel_presses(Items& items) {
    for (auto& item : items) {
        item.press = {};
        item.state_layer.animate_to(0.0);
    }
}

template <typename Items, typename HitRect, typename Select>
bool select_on_click(Items& items, const input::PointerEvent& e, std::size_t selected, HitRect hit, Select select) {
    const std::optional<std::size_t> clicked = update_presses(items, e, hit);
    if (clicked && *clicked != selected) {
        select(*clicked);
        return true;
    }
    return false;
}

}
