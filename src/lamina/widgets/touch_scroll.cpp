#include "lamina/style/style.hpp"
#include "lamina/widgets/touch_scroll.hpp"

#include <cmath>
#include <numbers>

namespace lamina::widgets {

namespace {

constexpr double degrees_per_half_turn = 180.0;

double sign_of(double v) {
    return v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
}

double drag_angle(double dx, double dy) {
    return std::atan2(std::abs(dy), std::abs(dx)) * degrees_per_half_turn / std::numbers::pi;
}

}

const motion::SplineDecay& fling_decay() {
    static const motion::SplineDecay decay(current_style().gesture.fling_decay);
    return decay;
}

bool angle_aligned(const tokens::TouchScrollTokens& t, input::ScrollAxes axis, double angle) {
    if (axis.both()) {
        return true;
    }
    if (axis.horizontal) {
        return angle <= t.horizontal_angle_limit_degrees;
    }
    if (axis.vertical) {
        return angle > t.horizontal_angle_limit_degrees && angle <= t.vertical_angle_limit_degrees;
    }
    return false;
}

TouchScroll::TouchScroll(const tokens::TouchScrollTokens& t) : tokens_(&t), tracker_x_(t.tracking), tracker_y_(t.tracking) {}

void TouchScroll::track(const input::PointerEvent& e) {
    tracker_x_.add(e.time_seconds, e.x_dp);
    tracker_y_.add(e.time_seconds, e.y_dp);
}

TouchScrollUpdate TouchScroll::press(const input::PointerEvent& e, input::ScrollAxes axes, bool catch_fling) {
    axes_ = axes;
    tracker_x_.reset();
    tracker_y_.reset();
    press_x_ = last_x_ = e.x_dp;
    press_y_ = last_y_ = e.y_dp;
    if (!axes.any()) {
        phase_ = Phase::idle;
        return {};
    }
    track(e);
    if (catch_fling) {
        phase_ = Phase::owning;
        return {.step = TouchScrollStep::caught};
    }
    phase_ = Phase::pending;
    return {};
}

TouchScrollUpdate TouchScroll::move(const input::PointerEvent& e) {
    if (phase_ == Phase::idle) {
        return {};
    }
    track(e);
    if (phase_ == Phase::owning) {
        TouchScrollUpdate u{.step = TouchScrollStep::drag};
        u.dx = axes_.horizontal ? e.x_dp - last_x_ : 0.0;
        u.dy = axes_.vertical ? e.y_dp - last_y_ : 0.0;
        last_x_ = e.x_dp;
        last_y_ = e.y_dp;
        return u;
    }
    const double dx = e.x_dp - press_x_;
    const double dy = e.y_dp - press_y_;
    const double slop = tokens_->slop;
    double over_x = 0.0;
    double over_y = 0.0;
    double travel = 0.0;
    if (axes_.both()) {
        travel = std::hypot(dx, dy);
        if (travel > 0.0) {
            over_x = dx - dx / travel * slop;
            over_y = dy - dy / travel * slop;
        }
    } else if (axes_.horizontal) {
        travel = std::abs(dx);
        over_x = dx - sign_of(dx) * slop;
    } else {
        travel = std::abs(dy);
        over_y = dy - sign_of(dy) * slop;
    }
    if (travel <= slop) {
        return {};
    }
    const double angle = drag_angle(dx, dy);
    const input::ScrollAxes waiting = e.waiting_scrollers;
    const bool outer_aligned = (waiting.horizontal && angle_aligned(*tokens_, {true, false}, angle)) ||
                               (waiting.vertical && angle_aligned(*tokens_, {false, true}, angle));
    if (!angle_aligned(*tokens_, axes_, angle) && outer_aligned) {
        return {};
    }
    phase_ = Phase::owning;
    last_x_ = e.x_dp;
    last_y_ = e.y_dp;
    return {.step = TouchScrollStep::claim, .dx = over_x, .dy = over_y};
}

TouchScrollUpdate TouchScroll::release(const input::PointerEvent& e) {
    if (phase_ != Phase::owning) {
        phase_ = Phase::idle;
        return {};
    }
    TouchScrollUpdate u;
    u.dx = axes_.horizontal ? e.x_dp - last_x_ : 0.0;
    u.dy = axes_.vertical ? e.y_dp - last_y_ : 0.0;
    phase_ = Phase::idle;
    tracker_x_.release(e.time_seconds);
    tracker_y_.release(e.time_seconds);
    u.step = TouchScrollStep::release;
    u.velocity_x = axes_.horizontal ? tracker_x_.velocity(tokens_->maximum_velocity) : 0.0;
    u.velocity_y = axes_.vertical ? tracker_y_.velocity(tokens_->maximum_velocity) : 0.0;
    return u;
}

void TouchScroll::yield() {
    phase_ = Phase::idle;
}

void TouchScroll::cancel() {
    phase_ = Phase::idle;
}

input::PointerEvent TouchScroll::forward(const input::PointerEvent& e) const {
    input::PointerEvent out = e;
    if (phase_ == Phase::pending) {
        out.waiting_scrollers = e.waiting_scrollers | axes_;
    }
    return out;
}

}
