#include "lamina/widgets/split_pane.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace lamina::widgets {

namespace {

constexpr const tokens::SplitPaneTokens& t = tokens::split_pane_tokens;

}

SplitPane::SplitPane(SplitPaneOptions options)
    : options_(std::move(options)),
      enabled_(options_.enabled),
      handle_painter_(options_.axis),
      handle_shown_(0.0, t.handle_fade) {
    if (options_.first_size) {
        first_ = *options_.first_size;
        sized_ = true;
    }
}

layout::Size SplitPane::measure(const layout::Constraints& c) {
    const double w = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : c.min_height;
    return c.constrain({w, h});
}

double SplitPane::available() const {
    return std::max(0.0, main(rect_) - t.divider_thickness);
}

double SplitPane::min_first() const {
    return std::min(options_.first_min, available());
}

double SplitPane::max_first() const {
    return std::max(min_first(), available() - options_.second_min);
}

double SplitPane::clamp_first(double size) const {
    return std::clamp(size, min_first(), max_first());
}

void SplitPane::arrange(const render::Rect& rect) {
    rect_ = rect;
    if (!sized_ && available() > 0.0) {
        first_ = available() * options_.first_fraction;
        sized_ = true;
    }
    place();
}

void SplitPane::place() {
    const double first = clamp_first(first_);
    const double divider = t.divider_thickness;
    if (options_.axis == layout::Axis::horizontal) {
        first_rect_ = {rect_.x, rect_.y, first, rect_.height};
        second_rect_ = {rect_.x + first + divider, rect_.y, std::max(0.0, rect_.width - first - divider), rect_.height};
    } else {
        first_rect_ = {rect_.x, rect_.y, rect_.width, first};
        second_rect_ = {rect_.x, rect_.y + first + divider, rect_.width, std::max(0.0, rect_.height - first - divider)};
    }
    if (options_.first) {
        options_.first->measure(layout::Constraints::tight({first_rect_.width, first_rect_.height}));
        options_.first->arrange(first_rect_);
    }
    if (options_.second) {
        options_.second->measure(layout::Constraints::tight({second_rect_.width, second_rect_.height}));
        options_.second->arrange(second_rect_);
    }
}

render::Rect SplitPane::divider_rect() const {
    const double first = clamp_first(first_);
    if (options_.axis == layout::Axis::horizontal) {
        return {rect_.x + first, rect_.y, t.divider_thickness, rect_.height};
    }
    return {rect_.x, rect_.y + first, rect_.width, t.divider_thickness};
}

render::Rect SplitPane::handle_rect() const {
    const render::Rect d = divider_rect();
    const double grow = (t.hit_thickness - t.divider_thickness) / 2.0;
    if (options_.axis == layout::Axis::horizontal) {
        return {d.x - grow, d.y, t.hit_thickness, d.height};
    }
    return {d.x, d.y - grow, d.width, t.hit_thickness};
}

void SplitPane::set_first_size(double size) {
    first_ = rect_.width > 0.0 || rect_.height > 0.0 ? clamp_first(size) : size;
    sized_ = true;
    if (rect_.width > 0.0 || rect_.height > 0.0) {
        place();
    }
}

std::optional<input::Cursor> SplitPane::cursor() const {
    if (!enabled() || (!hovered_ && !dragging_)) {
        return std::nullopt;
    }
    return options_.axis == layout::Axis::horizontal ? input::Cursor::resize_horizontal : input::Cursor::resize_vertical;
}

void SplitPane::set_focused(bool focused) {
    focus_.set_focused(focused && can_focus());
    retarget();
}

void SplitPane::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        set_focused(false);
        cancel_interaction();
    }
}

AccessibleNode SplitPane::accessible() const {
    return {AccessibleRole::separator,
            options_.accessible_name,
            divider_rect(),
            {.enabled = enabled(),
             .focusable = enabled(),
             .focused = focused(),
             .range = AccessibleRange{clamp_first(first_), min_first(), max_first()}},
            std::to_string(static_cast<int>(std::lround(clamp_first(first_))))};
}

void SplitPane::retarget() {
    handle_painter_.set_state(false, dragging_);
    handle_shown_.animate_to(hovered_ || dragging_ || focused() ? 1.0 : 0.0);
}

bool SplitPane::handle(const input::PointerEvent& e) {
    if (!enabled()) {
        return false;
    }
    focus_.release_on_press(e);
    if (e.action == input::PointerAction::leave) {
        hovered_ = false;
        dragging_ = false;
        retarget();
        return false;
    }
    const bool over = contains(handle_rect(), e.x_dp, e.y_dp);
    if (dragging_) {
        set_first_size(along(e) - origin() - grab_);
        if (e.action == input::PointerAction::up) {
            dragging_ = false;
            hovered_ = over;
        }
        retarget();
        return true;
    }
    hovered_ = over;
    if (e.action == input::PointerAction::down && over) {
        dragging_ = true;
        grab_ = along(e) - origin() - clamp_first(first_);
        restore_.reset();
        retarget();
        return true;
    }
    retarget();
    return false;
}

bool SplitPane::handle(const input::KeyEvent& e) {
    if (!focused() || !enabled() || e.ctrl) {
        return false;
    }
    const bool horizontal = options_.axis == layout::Axis::horizontal;
    const input::Key back = horizontal ? input::Key::left : input::Key::up;
    const input::Key forward = horizontal ? input::Key::right : input::Key::down;
    const double now = clamp_first(first_);
    if (e.key == back || e.key == forward) {
        set_first_size(now + (e.key == forward ? t.keyboard_step : -t.keyboard_step));
    } else if (e.key == input::Key::page_up || e.key == input::Key::page_down) {
        set_first_size(now + (e.key == input::Key::page_down ? t.page_step : -t.page_step));
    } else if (e.key == input::Key::home) {
        set_first_size(min_first());
    } else if (e.key == input::Key::end) {
        set_first_size(max_first());
    } else if (e.key == input::Key::enter && !e.repeat) {
        if (restore_ && now == min_first()) {
            set_first_size(*restore_);
            restore_.reset();
        } else {
            restore_ = now;
            set_first_size(min_first());
        }
        return true;
    } else {
        return false;
    }
    restore_.reset();
    return true;
}

void SplitPane::cancel_interaction() {
    hovered_ = false;
    dragging_ = false;
    retarget();
}

void SplitPane::step(double dt) {
    focus_.step(dt);
    enabled_.step(dt);
    handle_painter_.step(dt);
    handle_shown_.step(dt);
}

bool SplitPane::animating() const {
    return !focus_.settled() || !enabled_.settled() || !handle_painter_.settled() || !handle_shown_.settled();
}

void SplitPane::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect d = divider_rect();
    canvas.fill_rect(d, scheme[t.divider]);
    const render::Point centre{d.x + d.width / 2.0, d.y + d.height / 2.0};
    const double shown = std::clamp(handle_shown_.value(), 0.0, 1.0) * enabled_.amount();
    handle_painter_.paint(canvas, scheme, centre, shown);
    const render::Rect pill = handle_painter_.rect(centre);
    focus_.paint(canvas, scheme, pill, handle_painter_.radius(pill));
}

}
