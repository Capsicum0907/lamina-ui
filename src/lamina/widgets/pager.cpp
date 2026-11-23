#include "lamina/widgets/pager.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

constexpr double degrees_per_half_turn = 180.0;

double drag_angle(double dx, double dy) {
    return std::atan2(std::abs(dy), std::abs(dx)) * degrees_per_half_turn / std::numbers::pi;
}

double sign_of(double v) {
    return v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
}

}

Pager::Pager(PagerOptions options)
    : tokens_(options.tokens),
      name_(std::move(options.accessible_name)),
      mouse_drag_(options.mouse_drag),
      count_(options.count),
      tracker_(options.tokens->tracking) {
    scroll_to_page(options.page);
}

void Pager::set_count(std::size_t count) {
    count_ = count;
    if (count_ == 0) {
        cancel_interaction();
        position_ = 0.0;
        settled_ = 0;
        return;
    }
    if (settled_ >= count_ || target_ >= count_ || position_ > static_cast<double>(count_ - 1)) {
        scroll_to_page(std::min(settled_, count_ - 1));
    }
}

bool Pager::can_move(double finger_dx) const {
    if (count_ < 2) {
        return false;
    }
    if (finger_dx < 0.0) {
        return position_ < static_cast<double>(count_ - 1);
    }
    if (finger_dx > 0.0) {
        return position_ > 0.0;
    }
    return false;
}

void Pager::scroll_to_page(std::size_t page) {
    gesture_ = Gesture::idle;
    settling_ = false;
    settled_ = target_ = motion::nearest_page(static_cast<double>(page), count_);
    position_ = static_cast<double>(settled_);
}

void Pager::animate_to_page(std::size_t page) {
    if (count_ == 0) {
        return;
    }
    gesture_ = Gesture::idle;
    const std::size_t to = std::min(page, count_ - 1);
    const double jump = static_cast<double>(tokens_->animate_jump_pages);
    const double target = static_cast<double>(to);
    if (std::abs(target - position_) > jump) {
        position_ = target - sign_of(target - position_) * jump;
    }
    settle_to(to, 0.0, Source::request);
}

void Pager::settle_to(std::size_t page, double velocity, Source source) {
    target_ = page;
    source_ = source;
    settle_from_ = position_;
    spring_ = {position_, velocity};
    settling_ = position_ != static_cast<double>(page);
    if (!settling_) {
        finish_settle();
    }
}

void Pager::finish_settle() {
    settling_ = false;
    position_ = static_cast<double>(target_);
    settled_ = target_;
}

void Pager::follow(double finger_dx) {
    const double pitch = page_pitch();
    if (pitch <= 0.0) {
        return;
    }
    position_ = motion::clamp_position(position_ - finger_dx / pitch, count_);
}

void Pager::release(const input::PointerEvent& e) {
    follow(e.x_dp - last_x_);
    tracker_.add(e.time_seconds, e.x_dp);
    tracker_.release(e.time_seconds);
    const double velocity = tracker_.velocity(tokens_->maximum_velocity);
    gesture_ = Gesture::idle;
    const std::size_t page = motion::snap_page(position_, settled_, e.x_dp - press_x_, velocity, count_, tokens_->snap);
    const double pitch = page_pitch();
    settle_to(page, pitch > 0.0 ? -velocity / pitch : 0.0, Source::swipe);
}

bool Pager::accepts(const input::PointerEvent& e) const {
    return enabled_ && count_ > 1 && (e.kind != input::PointerKind::mouse || mouse_drag_);
}

Pager::Route Pager::begin(const input::PointerEvent& e) {
    if (e.action == input::PointerAction::down) {
        focus_.release_on_press(e);
    }
    if (e.action == input::PointerAction::leave) {
        if (gesture_ == Gesture::dragging) {
            settle_to(motion::snap_page(position_, settled_, 0.0, 0.0, count_, tokens_->snap), 0.0, Source::swipe);
        }
        gesture_ = Gesture::idle;
        return Route::deliver;
    }
    if (!accepts(e)) {
        return Route::deliver;
    }
    switch (e.action) {
    case input::PointerAction::down: {
        gesture_ = Gesture::idle;
        if (!contains(bounds_, e.x_dp, e.y_dp)) {
            return Route::deliver;
        }
        tracker_.reset();
        tracker_.add(e.time_seconds, e.x_dp);
        press_x_ = origin_x_ = last_x_ = e.x_dp;
        origin_y_ = e.y_dp;
        if (settling_) {
            if (std::abs(static_cast<double>(target_) - position_) * page_pitch() > tokens_->catch_distance) {
                settling_ = false;
                gesture_ = Gesture::dragging;
                return Route::claimed;
            }
            finish_settle();
        }
        gesture_ = Gesture::pending;
        return Route::deliver_then_check;
    }
    case input::PointerAction::move:
        if (gesture_ == Gesture::dragging) {
            follow(e.x_dp - last_x_);
            last_x_ = e.x_dp;
            tracker_.add(e.time_seconds, e.x_dp);
            return Route::consumed;
        }
        return gesture_ == Gesture::pending ? Route::deliver_then_check : Route::deliver;
    case input::PointerAction::up:
        if (gesture_ == Gesture::dragging) {
            release(e);
            return Route::consumed;
        }
        gesture_ = Gesture::idle;
        return Route::deliver;
    case input::PointerAction::leave:
        break;
    }
    return Route::deliver;
}

bool Pager::after_child(const input::PointerEvent& e, bool holds) {
    if (gesture_ != Gesture::pending) {
        return false;
    }
    if (holds) {
        gesture_ = Gesture::idle;
        return false;
    }
    if (e.action != input::PointerAction::move) {
        return false;
    }
    tracker_.add(e.time_seconds, e.x_dp);
    const double dx = e.x_dp - origin_x_;
    const double dy = e.y_dp - origin_y_;
    const double slop = tokens_->touch_slop;
    if (std::abs(dx) > slop && drag_angle(dx, dy) <= tokens_->horizontal_angle_limit_degrees && can_move(dx)) {
        settling_ = false;
        gesture_ = Gesture::dragging;
        follow(dx - sign_of(dx) * slop);
        last_x_ = e.x_dp;
        return true;
    }
    if (std::abs(dy) > slop) {
        gesture_ = Gesture::idle;
    }
    return false;
}

input::PointerEvent Pager::forward(const input::PointerEvent& e) const {
    input::PointerEvent out = e;
    if (gesture_ == Gesture::pending) {
        out.waiting_scrollers = e.waiting_scrollers | input::ScrollAxes{true, false};
    }
    return out;
}

bool Pager::handle(const input::KeyEvent& e) {
    if (!focus_.focused() || !enabled_ || count_ < 2 || e.ctrl || e.alt) {
        return false;
    }
    const std::size_t from = target_page();
    std::size_t to = from;
    switch (e.key) {
    case input::Key::left:
        to = from > 0 ? from - 1 : from;
        break;
    case input::Key::right:
        to = std::min(from + 1, count_ - 1);
        break;
    case input::Key::home:
        to = 0;
        break;
    case input::Key::end:
        to = count_ - 1;
        break;
    default:
        return false;
    }
    if (to == from) {
        return false;
    }
    animate_to_page(to);
    return true;
}

void Pager::cancel_interaction() {
    if (gesture_ == Gesture::dragging) {
        settle_to(motion::snap_page(position_, settled_, 0.0, 0.0, count_, tokens_->snap), 0.0, Source::swipe);
    }
    gesture_ = Gesture::idle;
}

void Pager::step(double dt) {
    focus_.step(dt);
    if (!settling_) {
        return;
    }
    const double target = static_cast<double>(target_);
    spring_ = motion::advance(tokens_->settle_spring, spring_, target, dt);
    const bool passed = (settle_from_ < target && spring_.value >= target) || (settle_from_ > target && spring_.value <= target);
    if (passed || motion::is_settled(spring_, target, tokens_->settle_threshold)) {
        finish_settle();
        return;
    }
    position_ = motion::clamp_position(spring_.value, count_);
}

void Pager::set_enabled(bool enabled) {
    if (enabled == enabled_) {
        return;
    }
    enabled_ = enabled;
    if (!enabled_) {
        cancel_interaction();
        focus_.set_focused(false);
    }
}

AccessibleNode Pager::accessible() const {
    AccessibleNode node{AccessibleRole::group, name_, bounds_, {.enabled = enabled_, .focusable = can_focus(), .focused = focused()}};
    if (count_ > 0) {
        node.state.range = AccessibleRange{static_cast<double>(settled_), 0.0, static_cast<double>(count_ - 1)};
    }
    return node;
}

void Pager::begin_page(render::Canvas& canvas, std::size_t page) const {
    canvas.save();
    canvas.push_clip(bounds_);
    canvas.translate(page_offset(page), 0.0);
}

void Pager::end_page(render::Canvas& canvas) const {
    canvas.pop_clip();
    canvas.restore();
}

void Pager::paint_focus(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    focus_.paint_inward(canvas, scheme, bounds_, tokens_->focus_ring_corner);
}

}
