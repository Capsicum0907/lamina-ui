#include "lamina/widgets/zoom_view.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "lamina/motion/lerp.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/touch_scroll.hpp"

namespace lamina::widgets {

namespace {

constexpr double degrees_per_half_turn = 180.0;
constexpr double right_angle_degrees = 90.0;

}

ZoomView::ZoomView(ZoomViewOptions options)
    : tokens_(options.tokens),
      name_(std::move(options.accessible_name)),
      content_width_(options.content_width),
      content_height_(options.content_height),
      tracker_x_(tokens().tracking),
      tracker_y_(tokens().tracking),
      fling_x_(fling_decay()),
      fling_y_(fling_decay()) {
    shown_ = raw_ = motion::fitted(geometry(), tokens().min_scale);
}

motion::ZoomGeometry ZoomView::geometry() const {
    return {bounds_.width, bounds_.height, content_width_, content_height_};
}

void ZoomView::set_bounds(const render::Rect& bounds) {
    const bool resized = bounds.width != bounds_.width || bounds.height != bounds_.height;
    const bool first = bounds_.width <= 0.0 || bounds_.height <= 0.0;
    bounds_ = bounds;
    if (!resized) {
        return;
    }
    if (first) {
        snap_to_fit();
        return;
    }
    if (!gesture_active()) {
        stop_motion();
        shown_ = raw_ = motion::clamp_pan(geometry(), {std::clamp(shown_.scale, tokens().min_scale, tokens().max_scale), shown_.x, shown_.y});
    }
}

void ZoomView::set_content_size(double width, double height) {
    content_width_ = width;
    content_height_ = height;
    snap_to_fit();
}

void ZoomView::snap_to_fit() {
    stop_motion();
    fingers_.clear();
    phase_ = Phase::idle;
    scaling_ = false;
    last_tap_.reset();
    drop_taps();
    shown_ = raw_ = motion::fitted(geometry(), tokens().min_scale);
}

void ZoomView::hold_tap(double x, double y, double up_seconds, double timeout_seconds) {
    pending_tap_ = PendingTap{{bounds_.x + x, bounds_.y + y}, up_seconds, timeout_seconds};
}

void ZoomView::end_tap_wait(double time_seconds) {
    if (pending_tap_ && time_seconds - pending_tap_->up_seconds > pending_tap_->timeout_seconds) {
        confirmed_tap_ = pending_tap_->at;
    }
    pending_tap_.reset();
}

void ZoomView::drop_taps() {
    pending_tap_.reset();
    confirmed_tap_.reset();
    click_.reset();
}

std::optional<render::Point> ZoomView::take_tap() {
    return std::exchange(confirmed_tap_, std::nullopt);
}

std::optional<double> ZoomView::next_wake_seconds() const {
    if (!pending_tap_) {
        return std::nullopt;
    }
    return std::max(0.0, pending_tap_->timeout_seconds - pending_tap_->waited_seconds);
}

void ZoomView::reset() {
    settle_to(motion::fitted(geometry(), tokens().min_scale));
}

void ZoomView::zoom_to(double scale, double x, double y) {
    const motion::ZoomTransform from = settling_ ? settle_target_ : shown_;
    const double fx = x - bounds_.x;
    const double fy = y - bounds_.y;
    const double s = std::clamp(scale, tokens().min_scale, tokens().max_scale);
    settle_to(motion::clamp_pan(geometry(), motion::zoom_about(from, fx, fy, s)));
}

bool ZoomView::zoomed() const {
    return shown_.scale > tokens().min_scale + tokens().zoomed_epsilon;
}

motion::PanEdges ZoomView::edges() const {
    return motion::pan_edges(geometry(), shown_, tokens().edge_tolerance);
}

bool ZoomView::can_pan_horizontally(double dx) const {
    const motion::PanEdges e = edges();
    return dx > 0.0 ? !e.left : (dx < 0.0 ? !e.right : false);
}

bool ZoomView::can_pan_vertically(double dy) const {
    const motion::PanEdges e = edges();
    return dy > 0.0 ? !e.top : (dy < 0.0 ? !e.bottom : false);
}

void ZoomView::set_enabled(bool enabled) {
    if (enabled == enabled_) {
        return;
    }
    enabled_ = enabled;
    if (!enabled_) {
        cancel_interaction();
    }
}

double ZoomView::focus_x() const {
    double sum = 0.0;
    for (const Finger& f : fingers_) {
        sum += f.x;
    }
    return fingers_.empty() ? 0.0 : sum / static_cast<double>(fingers_.size());
}

double ZoomView::focus_y() const {
    double sum = 0.0;
    for (const Finger& f : fingers_) {
        sum += f.y;
    }
    return fingers_.empty() ? 0.0 : sum / static_cast<double>(fingers_.size());
}

double ZoomView::span() const {
    if (fingers_.size() < 2) {
        return 0.0;
    }
    const double fx = focus_x();
    const double fy = focus_y();
    double dev_x = 0.0;
    double dev_y = 0.0;
    for (const Finger& f : fingers_) {
        dev_x += std::abs(f.x - fx);
        dev_y += std::abs(f.y - fy);
    }
    const double n = static_cast<double>(fingers_.size());
    return std::hypot(dev_x / n * 2.0, dev_y / n * 2.0);
}

motion::ZoomTransform ZoomView::banded(const motion::ZoomTransform& raw, double fx, double fy) const {
    const motion::ZoomGeometry g = geometry();
    const double s = motion::band_scale(raw.scale, scale_limits(), tokens().scale_band_coefficient, tokens().scale_band_ratio);
    motion::ZoomTransform z = motion::zoom_about(raw, fx, fy, s);
    z.x = motion::band_into(z.x, motion::pan_limits_x(g, s), {tokens().pan_band_coefficient, tokens().pan_band_extent * g.viewport_width});
    z.y = motion::band_into(z.y, motion::pan_limits_y(g, s), {tokens().pan_band_coefficient, tokens().pan_band_extent * g.viewport_height});
    return z;
}

motion::ZoomTransform ZoomView::unbanded(const motion::ZoomTransform& shown, double fx, double fy) const {
    const motion::ZoomGeometry g = geometry();
    const double s = shown.scale;
    const motion::ZoomTransform t{
        s,
        motion::unband_from(shown.x, motion::pan_limits_x(g, s), {tokens().pan_band_coefficient, tokens().pan_band_extent * g.viewport_width}),
        motion::unband_from(shown.y, motion::pan_limits_y(g, s), {tokens().pan_band_coefficient, tokens().pan_band_extent * g.viewport_height})};
    const double raw_scale = motion::unband_scale(s, scale_limits(), tokens().scale_band_coefficient, tokens().scale_band_ratio);
    return motion::zoom_about(t, fx, fy, raw_scale);
}

void ZoomView::stop_motion() {
    settling_ = false;
    fling_x_.stop();
    fling_y_.stop();
}

void ZoomView::catch_motion() {
    stop_motion();
}

void ZoomView::settle_to(const motion::ZoomTransform& target) {
    fling_x_.stop();
    fling_y_.stop();
    settle_from_ = shown_;
    settle_target_ = target;
    settle_ = {0.0, 0.0};
    settling_ = std::abs(target.scale - shown_.scale) > 0.0 || std::abs(target.x - shown_.x) > 0.0 || std::abs(target.y - shown_.y) > 0.0;
    if (!settling_) {
        shown_ = raw_ = target;
    }
}

void ZoomView::settle_about(double fx, double fy) {
    settle_to(motion::settle_target(geometry(), shown_, fx, fy, scale_limits()));
}

void ZoomView::release(double time_seconds, bool fling) {
    const motion::ZoomGeometry g = geometry();
    tracker_x_.release(time_seconds);
    tracker_y_.release(time_seconds);
    if (fling && zoomed() && motion::in_bounds(g, shown_, scale_limits(), tokens().edge_tolerance)) {
        shown_ = raw_ = motion::clamp_pan(g, shown_);
        const double vx = tracker_x_.velocity(tokens().maximum_velocity);
        const double vy = tracker_y_.velocity(tokens().maximum_velocity);
        const motion::ZoomLimits lx = motion::pan_limits_x(g, shown_.scale);
        const motion::ZoomLimits ly = motion::pan_limits_y(g, shown_.scale);
        settling_ = false;
        if (std::abs(vx) > tokens().minimum_fling_velocity && lx.max > lx.min) {
            fling_x_.start(shown_.x, vx);
        }
        if (std::abs(vy) > tokens().minimum_fling_velocity && ly.max > ly.min) {
            fling_y_.start(shown_.y, vy);
        }
        if (fling_x_.active() || fling_y_.active()) {
            return;
        }
    }
    settle_about(prev_focus_x_, prev_focus_y_);
}

void ZoomView::toggle_zoom(double fx, double fy) {
    if (zoomed()) {
        settle_to(motion::fitted(geometry(), tokens().min_scale));
        return;
    }
    settle_to(motion::clamp_pan(geometry(), motion::zoom_about(shown_, fx, fy, tokens().double_tap_scale)));
}

void ZoomView::yield() {
    phase_ = Phase::yielded;
    scaling_ = false;
    last_tap_.reset();
    pending_tap_.reset();
    settle_about(prev_focus_x_, prev_focus_y_);
}

bool ZoomView::hands_off(double dx, double dy) const {
    if (fingers_.size() != 1 || scaling_) {
        return false;
    }
    const double angle = std::atan2(std::abs(dy), std::abs(dx)) * degrees_per_half_turn / std::numbers::pi;
    const double limit = tokens().handoff_angle_degrees;
    if (angle <= limit) {
        return waiting_.horizontal && !can_pan_horizontally(dx);
    }
    if (angle >= right_angle_degrees - limit) {
        return waiting_.vertical && !can_pan_vertically(dy);
    }
    return false;
}

void ZoomView::regroup() {
    const bool was_scaling = scaling_;
    scaling_ = false;
    tracker_x_.reset();
    tracker_y_.reset();
    if (fingers_.empty()) {
        return;
    }
    prev_focus_x_ = start_focus_x_ = focus_x();
    prev_focus_y_ = start_focus_y_ = focus_y();
    initial_span_ = prev_span_ = span();
    if (was_scaling && fingers_.size() >= 2 && prev_span_ >= tokens().min_span) {
        scaling_ = true;
    }
}

std::vector<ZoomView::Finger>::iterator ZoomView::find_finger(std::uint64_t id) {
    return std::find_if(fingers_.begin(), fingers_.end(), [id](const Finger& f) { return f.id == id; });
}

bool ZoomView::finger_down(const input::FingerEvent& e) {
    const double x = e.x_dp - bounds_.x;
    const double y = e.y_dp - bounds_.y;
    if (const auto stale = find_finger(e.finger); stale != fingers_.end()) {
        fingers_.erase(stale);
        if (fingers_.empty()) {
            phase_ = Phase::idle;
            scaling_ = false;
        }
    }
    if (fingers_.empty()) {
        end_tap_wait(e.time_seconds);
        if (!contains(bounds_, e.x_dp, e.y_dp)) {
            return false;
        }
        catch_motion();
        fingers_.push_back({e.finger, x, y, x, y});
        waiting_ = {};
        const bool second_tap = last_tap_ && e.time_seconds - last_tap_->up_seconds <= tokens().double_tap_timeout_seconds &&
                                e.time_seconds - last_tap_->up_seconds >= tokens().double_tap_min_seconds &&
                                std::hypot(x - last_tap_->x, y - last_tap_->y) < tokens().double_tap_slop;
        if (second_tap) {
            const Tap tap = *last_tap_;
            last_tap_.reset();
            phase_ = Phase::double_tap;
            single_tap_ = false;
            toggle_zoom(tap.x, tap.y);
            return true;
        }
        last_tap_.reset();
        raw_ = unbanded(shown_, x, y);
        phase_ = Phase::pending;
        single_tap_ = true;
        regroup();
        tracker_x_.add(e.time_seconds, x);
        tracker_y_.add(e.time_seconds, y);
        return zoomed();
    }
    fingers_.push_back({e.finger, x, y, x, y});
    single_tap_ = false;
    if (phase_ == Phase::yielded || phase_ == Phase::double_tap) {
        return false;
    }
    if (phase_ == Phase::handed) {
        catch_motion();
        phase_ = Phase::pending;
    }
    regroup();
    return true;
}

bool ZoomView::finger_move(const input::FingerEvent& e) {
    const auto it = find_finger(e.finger);
    if (it == fingers_.end()) {
        return false;
    }
    it->x = e.x_dp - bounds_.x;
    it->y = e.y_dp - bounds_.y;
    if (phase_ == Phase::yielded || phase_ == Phase::double_tap || phase_ == Phase::handed) {
        return phase_ == Phase::double_tap;
    }
    if (single_tap_ && std::hypot(it->x - it->down_x, it->y - it->down_y) > tokens().double_tap_touch_slop) {
        single_tap_ = false;
    }
    const double fx = focus_x();
    const double fy = focus_y();
    const double sp = span();
    if (fingers_.size() >= 2) {
        if (!scaling_ && sp >= tokens().min_span && std::abs(sp - initial_span_) > tokens().span_slop) {
            scaling_ = true;
            prev_span_ = sp;
        } else if (scaling_ && sp < tokens().min_span) {
            scaling_ = false;
            initial_span_ = sp;
        }
    }
    if (phase_ == Phase::pending) {
        const double travel = std::hypot(fx - start_focus_x_, fy - start_focus_y_);
        const bool can_pan = fingers_.size() >= 2 || zoomed();
        if (scaling_) {
            phase_ = Phase::panning;
        } else if (can_pan && travel > tokens().touch_slop) {
            if (hands_off(fx - start_focus_x_, fy - start_focus_y_)) {
                phase_ = Phase::handed;
                last_tap_.reset();
                return false;
            }
            phase_ = Phase::panning;
            prev_focus_x_ = start_focus_x_ + (fx - start_focus_x_) * tokens().touch_slop / travel;
            prev_focus_y_ = start_focus_y_ + (fy - start_focus_y_) * tokens().touch_slop / travel;
        }
    }
    tracker_x_.add(e.time_seconds, fx);
    tracker_y_.add(e.time_seconds, fy);
    if (phase_ != Phase::panning) {
        prev_span_ = sp;
        return false;
    }
    const double factor = scaling_ && prev_span_ > 0.0 ? sp / prev_span_ : 1.0;
    raw_ = motion::pan_by(motion::zoom_about(raw_, prev_focus_x_, prev_focus_y_, raw_.scale * factor), fx - prev_focus_x_, fy - prev_focus_y_);
    shown_ = banded(raw_, fx, fy);
    prev_focus_x_ = fx;
    prev_focus_y_ = fy;
    prev_span_ = sp;
    return true;
}

bool ZoomView::finger_up(const input::FingerEvent& e) {
    const auto it = find_finger(e.finger);
    if (it == fingers_.end()) {
        return false;
    }
    const Finger lifted = *it;
    fingers_.erase(it);
    if (!fingers_.empty()) {
        if (phase_ != Phase::yielded && phase_ != Phase::double_tap && phase_ != Phase::handed) {
            regroup();
        }
        return true;
    }
    const Phase was = phase_;
    phase_ = Phase::idle;
    scaling_ = false;
    if (was == Phase::double_tap || was == Phase::yielded || was == Phase::handed) {
        return was == Phase::double_tap;
    }
    const bool cancelled = e.action == input::FingerAction::cancel;
    if (was == Phase::pending && single_tap_ && !cancelled) {
        last_tap_ = Tap{lifted.down_x, lifted.down_y, e.time_seconds};
        hold_tap(lifted.down_x, lifted.down_y, e.time_seconds, tokens().double_tap_timeout_seconds);
    }
    single_tap_ = false;
    release(e.time_seconds, was == Phase::panning && !cancelled);
    return was == Phase::panning;
}

bool ZoomView::handle(const input::FingerEvent& e) {
    if (!enabled_) {
        return false;
    }
    switch (e.action) {
    case input::FingerAction::down: return finger_down(e);
    case input::FingerAction::move: return finger_move(e);
    case input::FingerAction::up:
    case input::FingerAction::cancel: return finger_up(e);
    }
    return false;
}

bool ZoomView::handle(const input::PointerEvent& e) {
    if (!enabled_) {
        return false;
    }
    if (e.action == input::PointerAction::leave) {
        click_.reset();
        if (phase_ == Phase::mouse) {
            phase_ = Phase::idle;
            settle_about(mouse_x_, mouse_y_);
        }
        if (!after_touch_up_ && !fingers_.empty() && phase_ != Phase::yielded) {
            yield();
        }
        after_touch_up_ = false;
        return false;
    }
    after_touch_up_ = e.kind == input::PointerKind::touch && e.action == input::PointerAction::up;
    if (e.kind == input::PointerKind::touch) {
        if (e.action == input::PointerAction::down || e.action == input::PointerAction::move) {
            waiting_ = e.waiting_scrollers;
        }
        return holds_pointer();
    }
    const double x = e.x_dp - bounds_.x;
    const double y = e.y_dp - bounds_.y;
    switch (e.action) {
    case input::PointerAction::down:
        click_.reset();
        if (!contains(bounds_, e.x_dp, e.y_dp) || !fingers_.empty()) {
            end_tap_wait(e.time_seconds);
            return false;
        }
        if (e.clicks >= 2) {
            pending_tap_.reset();
            toggle_zoom(x, y);
            return true;
        }
        end_tap_wait(e.time_seconds);
        click_ = render::Point{x, y};
        if (!zoomed()) {
            return false;
        }
        catch_motion();
        raw_ = unbanded(shown_, x, y);
        phase_ = Phase::mouse;
        mouse_x_ = x;
        mouse_y_ = y;
        return true;
    case input::PointerAction::move:
        if (click_ && std::hypot(x - click_->x, y - click_->y) > tokens().double_tap_touch_slop) {
            click_.reset();
        }
        if (phase_ != Phase::mouse) {
            return false;
        }
        raw_ = motion::pan_by(raw_, x - mouse_x_, y - mouse_y_);
        shown_ = banded(raw_, x, y);
        mouse_x_ = x;
        mouse_y_ = y;
        return true;
    case input::PointerAction::up:
        if (click_) {
            hold_tap(click_->x, click_->y, e.time_seconds, tokens().double_click_timeout_seconds);
            click_.reset();
        }
        if (phase_ != Phase::mouse) {
            return false;
        }
        phase_ = Phase::idle;
        settle_about(x, y);
        return true;
    case input::PointerAction::leave: break;
    }
    return false;
}

bool ZoomView::handle(const input::WheelEvent& e) {
    if (!enabled_ || !e.ctrl || e.dy == 0.0 || !contains(bounds_, e.x_dp, e.y_dp) || gesture_active()) {
        return false;
    }
    const motion::ZoomTransform from = settling_ ? settle_target_ : shown_;
    zoom_to(from.scale * std::pow(tokens().wheel_zoom_factor, -e.dy), e.x_dp, e.y_dp);
    return true;
}

void ZoomView::cancel_interaction() {
    const bool active = gesture_active();
    fingers_.clear();
    phase_ = Phase::idle;
    scaling_ = false;
    single_tap_ = false;
    after_touch_up_ = false;
    waiting_ = {};
    last_tap_.reset();
    drop_taps();
    if (active) {
        settle_about(prev_focus_x_, prev_focus_y_);
    }
}

bool ZoomView::holds_pointer() const {
    if (phase_ == Phase::mouse) {
        return true;
    }
    if (fingers_.empty() || phase_ == Phase::yielded || phase_ == Phase::handed) {
        return false;
    }
    return phase_ == Phase::double_tap || phase_ == Phase::panning || fingers_.size() >= 2;
}

void ZoomView::restyle() {
    style_stamp_.refresh();
    fling_x_.set_decay(fling_decay());
    fling_y_.set_decay(fling_decay());
}

void ZoomView::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    if (pending_tap_) {
        if (pending_tap_->fresh) {
            pending_tap_->fresh = false;
        } else {
            pending_tap_->waited_seconds += dt;
        }
        if (pending_tap_->waited_seconds >= pending_tap_->timeout_seconds) {
            confirmed_tap_ = pending_tap_->at;
            pending_tap_.reset();
            last_tap_.reset();
        }
    }
    const motion::ZoomGeometry g = geometry();
    if (fling_x_.active() || fling_y_.active()) {
        const motion::ZoomLimits lx = motion::pan_limits_x(g, shown_.scale);
        const motion::ZoomLimits ly = motion::pan_limits_y(g, shown_.scale);
        if (fling_x_.active()) {
            const double v = fling_x_.step(dt);
            shown_.x = std::clamp(v, lx.min, lx.max);
            if (std::abs(v - shown_.x) > tokens().edge_tolerance) {
                fling_x_.stop();
            }
        }
        if (fling_y_.active()) {
            const double v = fling_y_.step(dt);
            shown_.y = std::clamp(v, ly.min, ly.max);
            if (std::abs(v - shown_.y) > tokens().edge_tolerance) {
                fling_y_.stop();
            }
        }
        raw_ = shown_;
    }
    if (settling_) {
        settle_ = motion::advance(tokens().settle_spring, settle_, 1.0, dt);
        if (motion::is_settled(settle_, 1.0, tokens().settle_threshold)) {
            settling_ = false;
            shown_ = settle_target_;
        } else {
            const double p = settle_.value;
            shown_ = {motion::lerp(settle_from_.scale, settle_target_.scale, p), motion::lerp(settle_from_.x, settle_target_.x, p),
                      motion::lerp(settle_from_.y, settle_target_.y, p)};
        }
        raw_ = shown_;
    }
}

bool ZoomView::animating() const {
    return settling_ || fling_x_.active() || fling_y_.active();
}

std::optional<input::Cursor> ZoomView::cursor() const {
    if (phase_ == Phase::mouse) {
        return input::Cursor::move;
    }
    return std::nullopt;
}

std::optional<AccessibleNode> ZoomView::accessible() const {
    AccessibleNode node{AccessibleRole::group, name_, bounds_};
    node.state.enabled = enabled_;
    node.state.range = AccessibleRange{shown_.scale, tokens().min_scale, tokens().max_scale};
    return node;
}

void ZoomView::begin_content(render::Canvas& canvas) const {
    canvas.save();
    canvas.push_clip(bounds_);
    canvas.translate(bounds_.x + shown_.x, bounds_.y + shown_.y);
    canvas.scale(motion::fit_scale(geometry()) * shown_.scale);
}

void ZoomView::end_content(render::Canvas& canvas) const {
    canvas.pop_clip();
    canvas.restore();
}

}
