#include "lamina/widgets/pull_to_refresh.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

bool inside(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

}

double pull_offset(double pulled, double threshold, double max_tension) {
    if (pulled <= threshold || threshold <= 0.0) {
        return std::max(0.0, pulled);
    }
    const double linear = std::clamp(pulled / threshold - 1.0, 0.0, max_tension);
    const double tension = linear - linear * linear / (2.0 * max_tension);
    return threshold + threshold * tension;
}

double pull_distance(double offset, double threshold, double max_tension) {
    if (offset <= threshold || threshold <= 0.0) {
        return std::max(0.0, offset);
    }
    const double top = max_tension / 2.0;
    const double tension = std::min(offset / threshold - 1.0, top);
    const double linear = max_tension * (1.0 - std::sqrt(std::max(0.0, 1.0 - 2.0 * tension / max_tension)));
    return threshold * (1.0 + linear);
}

PullToRefresh::PullToRefresh(PullToRefreshOptions options)
    : options_(std::move(options)),
      distance_(0.0, options_.tokens->settle_spring),
      indicator_(LoadingIndicatorOptions{.progress = 0.0, .contained = true, .size = options_.tokens->container_size}) {}

bool PullToRefresh::shown() const {
    return refreshing_ || distance_.value() > 0.0;
}

void PullToRefresh::set_enabled(bool enabled) {
    if (!enabled && pulling_) {
        PullToRefreshResponse ignored;
        release(false, ignored);
    }
    if (!enabled) {
        touch_.reset();
    }
    options_.enabled = enabled;
}

render::Rect PullToRefresh::container_rect() const {
    const double size = t().container_size;
    return {area_.x + (area_.width - size) / 2.0, area_.y + distance_.value() * t().threshold - size, size, size};
}

double PullToRefresh::indicator_scale() const {
    const double f = std::clamp(distance_.value(), 0.0, 1.0);
    return t().start_scale + (1.0 - t().start_scale) * f;
}

void PullToRefresh::sync_indicator() {
    if (indicator_.determinate()) {
        indicator_.set_progress(std::clamp(distance_.value(), 0.0, 1.0));
    }
}

void PullToRefresh::follow(double y) {
    const double pulled = std::max(0.0, y - *touch_->anchor_y - t().slop) * t().drag_multiplier;
    const double f = pull_offset(pulled, t().threshold, t().max_tension) / t().threshold;
    distance_.snap_to(f);
    turn_ = std::max(0.0, f - 1.0) * t().overshoot_turn_degrees;
    sync_indicator();
}

void PullToRefresh::release(bool commit, PullToRefreshResponse& response) {
    pulling_ = false;
    if (commit && !refreshing_ && distance_.value() >= 1.0) {
        refreshing_ = true;
        response.refresh = true;
        indicator_.set_progress(std::nullopt);
    }
    distance_.animate_to(refreshing_ ? 1.0 : 0.0);
}

PullToRefreshResponse PullToRefresh::handle(const input::PointerEvent& e, bool at_top) {
    PullToRefreshResponse r;
    if (!options_.enabled) {
        return r;
    }
    if (e.action == input::PointerAction::leave) {
        touch_.reset();
        if (pulling_) {
            release(false, r);
            r.consumed = true;
        }
        return r;
    }
    if (e.kind != input::PointerKind::touch) {
        return r;
    }
    switch (e.action) {
    case input::PointerAction::down:
        if (!pulling_ && inside(area_, e.x_dp, e.y_dp)) {
            touch_ = Touch{e.x_dp, e.y_dp, at_top ? std::optional(e.y_dp) : std::nullopt, e.x_dp};
        }
        return r;
    case input::PointerAction::move: {
        if (!touch_) {
            return r;
        }
        if (pulling_) {
            follow(e.y_dp);
            r.consumed = true;
            return r;
        }
        if (!at_top) {
            touch_->anchor_y.reset();
            return r;
        }
        if (!touch_->anchor_y || e.y_dp < *touch_->anchor_y) {
            touch_->anchor_y = e.y_dp;
            touch_->anchor_x = e.x_dp;
            return r;
        }
        const double dy = e.y_dp - *touch_->anchor_y;
        const double dx = std::abs(e.x_dp - touch_->anchor_x);
        const double limit = std::tan(t().max_angle_degrees * std::numbers::pi / 180.0);
        if (refreshing_ || dy <= t().slop || dx > dy * limit) {
            return r;
        }
        if (distance_.value() > 0.0) {
            const double start = pull_distance(distance_.value() * t().threshold, t().threshold, t().max_tension);
            touch_->anchor_y = e.y_dp - t().slop - start / t().drag_multiplier;
        }
        pulling_ = true;
        follow(e.y_dp);
        r.claimed = true;
        r.consumed = true;
        return r;
    }
    case input::PointerAction::up:
        touch_.reset();
        if (pulling_) {
            release(true, r);
            r.consumed = true;
        }
        return r;
    case input::PointerAction::leave:
        break;
    }
    return r;
}

void PullToRefresh::cancel_interaction() {
    touch_.reset();
    if (pulling_) {
        PullToRefreshResponse ignored;
        release(false, ignored);
    }
}

void PullToRefresh::set_refreshing(bool refreshing) {
    if (refreshing == refreshing_) {
        return;
    }
    refreshing_ = refreshing;
    if (refreshing) {
        pulling_ = false;
        touch_.reset();
        if (indicator_.determinate()) {
            indicator_.set_progress(std::nullopt);
        }
        distance_.animate_to(1.0);
        return;
    }
    if (!pulling_) {
        distance_.animate_to(0.0);
    }
}

std::optional<AccessibleNode> PullToRefresh::accessible() const {
    if (!shown()) {
        return std::nullopt;
    }
    AccessibleNode node{AccessibleRole::progress_indicator, options_.accessible_name, container_rect()};
    if (const auto p = indicator_.progress()) {
        node.state.range = AccessibleRange{*p, 0.0, 1.0};
    }
    return node;
}

void PullToRefresh::step(double dt) {
    distance_.step(dt);
    indicator_.step(dt);
    if (!pulling_) {
        sync_indicator();
    }
    if (!refreshing_ && !pulling_ && distance_.settled() && distance_.value() <= 0.0) {
        if (!indicator_.determinate()) {
            indicator_ = LoadingIndicator(LoadingIndicatorOptions{.progress = 0.0, .contained = true, .size = t().container_size});
        }
        turn_ = 0.0;
    }
}

bool PullToRefresh::animating() const {
    return !distance_.settled() || (shown() && indicator_.animating());
}

void PullToRefresh::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (distance_.value() <= 0.0) {
        return;
    }
    const render::Rect r = container_rect();
    if (r.y + r.height <= area_.y) {
        return;
    }
    const double size = t().container_size;
    canvas.save();
    canvas.push_clip(area_);
    paint_surface(canvas, r, tokens::corner_radius(t().container_shape, size, size), scheme[t().container],
                  tokens::elevation_dp[t().elevation_level], scheme[t().shadow]);
    canvas.translate(r.x + size / 2.0, r.y + size / 2.0);
    canvas.rotate(turn_ * std::numbers::pi / 180.0);
    canvas.scale(indicator_scale());
    canvas.translate(-size / 2.0, -size / 2.0);
    canvas.fill_path(indicator_.indicator_path(), scheme[t().indicator]);
    canvas.pop_clip();
    canvas.restore();
}

}
