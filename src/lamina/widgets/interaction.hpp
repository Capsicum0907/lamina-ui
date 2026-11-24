#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

#include "lamina/input/events.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"

namespace lamina::widgets {

constexpr bool contains(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

constexpr render::Rect touch_target(const render::Rect& r, double minimum = current_style().interaction.minimum_interactive_size) {
    const double w = std::max(r.width, minimum);
    const double h = std::max(r.height, minimum);
    return {r.x - (w - r.width) / 2.0, r.y - (h - r.height) / 2.0, w, h};
}

constexpr render::Rect hit_area(const input::PointerEvent& e, const render::Rect& r, double minimum = current_style().interaction.minimum_interactive_size) {
    return e.margin_claimed ? r : touch_target(r, minimum);
}

struct TargetHit {
    bool inside = false;
    double distance_squared = 0.0;

    constexpr bool beats(const TargetHit& other) const {
        if (inside != other.inside) {
            return inside;
        }
        return distance_squared < other.distance_squared;
    }
};

constexpr std::optional<TargetHit> target_hit(const render::Rect& r, const render::Rect& area, double x, double y) {
    if (contains(r, x, y)) {
        return TargetHit{true, 0.0};
    }
    if (!contains(area, x, y)) {
        return std::nullopt;
    }
    const double dx = std::max({r.x - x, x - (r.x + r.width), 0.0});
    const double dy = std::max({r.y - y, y - (r.y + r.height), 0.0});
    return TargetHit{false, dx * dx + dy * dy};
}

constexpr std::optional<TargetHit> target_hit(const render::Rect& r, double x, double y, double minimum = current_style().interaction.minimum_interactive_size) {
    return target_hit(r, touch_target(r, minimum), x, y);
}

constexpr std::optional<TargetHit> best_hit(std::optional<TargetHit> a, std::optional<TargetHit> b) {
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    return b->beats(*a) ? b : a;
}

class TargetArbiter {
public:
    template <typename Candidates>
    void resolve(const input::PointerEvent& e, Candidates candidates) {
        passing_ = e.margin_claimed || e.action == input::PointerAction::leave;
        const bool continuing = gesture_ && (e.action == input::PointerAction::move || e.action == input::PointerAction::up);
        if (passing_) {
            current_ = nullptr;
        } else if (continuing) {
            current_ = gesture_winner_;
        } else {
            current_ = nullptr;
            std::optional<TargetHit> best;
            candidates([&](const void* key, const std::optional<TargetHit>& h) {
                if (h && (!best || !best->beats(*h))) {
                    best = h;
                    current_ = key;
                }
            });
        }
        if (e.action == input::PointerAction::down) {
            gesture_ = true;
            gesture_winner_ = current_;
        } else if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave) {
            gesture_ = false;
            gesture_winner_ = nullptr;
        }
    }

    input::PointerEvent routed(const input::PointerEvent& e, const void* key) const {
        if (passing_ || current_ == nullptr || key == current_) {
            return e;
        }
        input::PointerEvent lost = e;
        lost.margin_claimed = true;
        return lost;
    }

    const void* winner() const { return current_; }

private:
    const void* current_ = nullptr;
    const void* gesture_winner_ = nullptr;
    bool gesture_ = false;
    bool passing_ = false;
};

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
            return current_style().state_layer.pressed;
        }
        return hovered ? current_style().state_layer.hover : 0.0;
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
