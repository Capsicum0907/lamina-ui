#include "lamina/widgets/content_host.hpp"

#include <algorithm>
#include <functional>
#include <limits>

namespace lamina::widgets {

namespace {

constexpr input::PointerEvent pressed_elsewhere{input::PointerAction::down, -std::numeric_limits<double>::infinity(),
                                                -std::numeric_limits<double>::infinity()};

}

ContentHost::ContentHost(std::optional<ContentSlot> slot) : slot_(std::move(slot)) {}

double ContentHost::measure(double width) {
    if (empty()) {
        return 0.0;
    }
    if (width != measured_width_) {
        height_ = slot_->node->measure({width, width, 0.0, layout::unbounded}).height;
        measured_width_ = width;
        arranged_width_ = -1.0;
    }
    return height_;
}

void ContentHost::place(const render::Rect& viewport) {
    if (empty()) {
        scroll_.set_viewport(viewport);
        return;
    }
    measure(viewport.width);
    if (arranged_width_ != viewport.width) {
        slot_->node->arrange({0.0, 0.0, viewport.width, height_});
        arranged_width_ = viewport.width;
    }
    scroll_.set_viewport(viewport);
    scroll_.set_content_height(height_);
}

void ContentHost::clear_focus() {
    if (focus()) {
        focus()->clear_focus();
    }
}

void ContentHost::reveal_focus() {
    if (!focus()) {
        return;
    }
    if (const std::optional<render::Rect> b = focus()->focused_bounds()) {
        scroll_.ensure_visible(b->y, b->y + b->height);
    }
}

bool ContentHost::holds_pointer() const {
    return scroll_.dragging() || (widgets() && widgets()->holds_pointer());
}

bool ContentHost::handle(const input::PointerEvent& e) {
    if (empty()) {
        return false;
    }
    bool used = false;
    const bool claimed = scroll_.route(e, [&](const input::PointerEvent& pe) {
        if (!widgets()) {
            return false;
        }
        const input::PointerEvent inside = scroll_.to_content(pe);
        if (pe.action == input::PointerAction::down && inside.action == input::PointerAction::leave) {
            widgets()->handle(pressed_elsewhere);
            return false;
        }
        used = widgets()->handle(inside);
        return widgets()->holds_pointer();
    });
    return claimed || used;
}

bool ContentHost::handle(const input::WheelEvent& e) {
    return !empty() && scroll_.handle(e);
}

bool ContentHost::handle(const input::KeyEvent& e) {
    if (empty()) {
        return false;
    }
    if (widgets() && (!focus() || focused())) {
        const bool consumed = widgets()->handle(e);
        reveal_focus();
        return consumed;
    }
    return scroll_.handle(e);
}

bool ContentHost::handle(const input::TextEvent& e) {
    if (!widgets() || (focus() && !focused())) {
        return false;
    }
    widgets()->handle(e);
    reveal_focus();
    return true;
}

void ContentHost::press_elsewhere() {
    if (widgets()) {
        widgets()->handle(pressed_elsewhere);
    }
}

void ContentHost::cancel_interaction() {
    scroll_.cancel_interaction();
    if (widgets()) {
        widgets()->cancel_interaction();
    }
}

void ContentHost::step(double dt) {
    if (empty()) {
        return;
    }
    scroll_.step(dt);
    if (const std::optional<input::PointerEvent> hover = scroll_.rehover(); hover && widgets()) {
        widgets()->handle(*hover);
    }
    if (widgets()) {
        widgets()->step(dt);
    }
}

bool ContentHost::animating() const {
    return !empty() && (scroll_.animating() || (widgets() && widgets()->animating()));
}

void ContentHost::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (empty() || !widgets()) {
        return;
    }
    scroll_.begin_content(canvas);
    widgets()->paint(canvas, scheme);
    scroll_.end_content(canvas);
    scroll_.paint(canvas, scheme);
}

void ContentHost::collect(std::vector<AccessibleNode>& out) const {
    if (empty() || !widgets()) {
        return;
    }
    std::vector<AccessibleNode> nodes = widgets()->accessible_nodes();
    std::function<void(AccessibleNode&)> shift = [&](AccessibleNode& n) {
        n.bounds = scroll_.to_window(n.bounds);
        for (AccessibleNode& c : n.children) {
            shift(c);
        }
    };
    for (AccessibleNode& n : nodes) {
        shift(n);
        out.push_back(std::move(n));
    }
}

std::optional<render::Rect> ContentHost::caret_area() const {
    if (!focused()) {
        return std::nullopt;
    }
    const std::optional<render::Rect> caret = focus()->caret_area();
    return caret ? std::optional<render::Rect>(scroll_.to_window(*caret)) : std::nullopt;
}

std::optional<double> ContentHost::next_wake_seconds() const {
    std::optional<double> wake;
    if (focus()) {
        wake = focus()->next_wake_seconds();
    }
    if (const double bar = scroll_.next_wake_seconds(); bar >= 0.0) {
        wake = wake ? std::min(*wake, bar) : bar;
    }
    return wake;
}

}
