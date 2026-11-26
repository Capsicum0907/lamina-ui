#include "lamina/style/style.hpp"
#include "lamina/widgets/pull_to_refresh.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/surface.hpp"
#include "lamina/widgets/wavy_progress.hpp"

namespace lamina::widgets {

namespace {

bool inside(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

double radians(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

render::Point on_circle(render::Point c, double radius, double degrees) {
    const double a = radians(degrees);
    return {c.x + radius * std::cos(a), c.y + radius * std::sin(a)};
}

int arc_segments(double sweep_degrees, int per_turn) {
    return std::max(1, static_cast<int>(std::ceil(std::abs(sweep_degrees) / 360.0 * per_turn)));
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

PullArrow pull_arrow(double fraction, const tokens::PullArrowTokens& k) {
    const double adjusted = std::max(std::min(1.0, fraction) - k.arc_start, 0.0) * k.arc_gain;
    const double linear = std::clamp(std::abs(fraction) - 1.0, 0.0, k.max_tension);
    const double tension = linear - linear * linear / (2.0 * k.max_tension);
    const double rotation = (k.rotation_start + k.rotation_arc * adjusted + tension) * k.rotation_scale;
    return {rotation * 360.0, (rotation + adjusted * k.max_arc) * 360.0, std::min(1.0, adjusted)};
}

PullToRefresh::PullToRefresh(PullToRefreshOptions options)
    : options_(std::move(options)),
      distance_(0.0, t().settle_spring),
      indicator_(LoadingIndicatorOptions{.progress = 0.0, .contained = true, .size = t().container_size}),
      arrow_alpha_(a().min_alpha) {}

void PullToRefresh::set_style(PullIndicatorStyle style) {
    options_.style = style;
}

double PullToRefresh::container_size() const {
    return options_.style == PullIndicatorStyle::arrow ? a().container_size : t().container_size;
}

double PullToRefresh::spinner_sweep_degrees() const {
    return circular_indeterminate_sweep(spinner_ms_) * 360.0;
}

double PullToRefresh::spinner_rotation_degrees() const {
    return circular_indeterminate_rotation(spinner_ms_) + current_style().wavy_progress.start_angle_degrees;
}

bool PullToRefresh::shown() const {
    return spinning() || distance_.value() > 0.0;
}

void PullToRefresh::start_refreshing() {
    refreshing_ = true;
    holding_ = false;
    refresh_seconds_ = 0.0;
    if (indicator_.determinate()) {
        indicator_.set_progress(std::nullopt);
    }
    spinner_mix_.animate_to(1.0, a().crossfade);
    distance_.animate_to(1.0);
}

void PullToRefresh::leave() {
    if (refresh_seconds_ < t().min_refresh_seconds) {
        holding_ = true;
        return;
    }
    holding_ = false;
    if (!pulling_) {
        distance_.animate_to(0.0);
    }
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
    const double size = container_size();
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
    if (commit && !spinning() && distance_.value() >= 1.0) {
        response.refresh = true;
        start_refreshing();
        return;
    }
    distance_.animate_to(spinning() ? 1.0 : 0.0);
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
        if (spinning() || dy <= t().slop || dx > dy * limit) {
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
    if (refreshing) {
        pulling_ = false;
        touch_.reset();
        if (holding_) {
            refreshing_ = true;
            holding_ = false;
            return;
        }
        start_refreshing();
        return;
    }
    refreshing_ = false;
    leave();
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

void PullToRefresh::restyle() {
    style_stamp_.refresh();
    distance_.set_spec(t().settle_spring);
    indicator_.restyle();
    indicator_.set_size(t().container_size);
    arrow_alpha_.retime(a().alpha_tween);
    spinner_mix_.retime(a().crossfade);
}

void PullToRefresh::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    if (spinning()) {
        refresh_seconds_ += dt;
    }
    arrow_alpha_.animate_to(distance_.value() >= 1.0 ? a().max_alpha : a().min_alpha, a().alpha_tween);
    arrow_alpha_.step(dt);
    spinner_mix_.step(dt);
    if (spinner_mix_.value() > 0.0) {
        spinner_ms_ += dt * 1000.0;
    }
    if (holding_ && refresh_seconds_ >= t().min_refresh_seconds) {
        leave();
    }
    distance_.step(dt);
    indicator_.step(dt);
    if (!pulling_) {
        sync_indicator();
    }
    if (!spinning() && !pulling_ && distance_.settled() && distance_.value() <= 0.0) {
        if (!indicator_.determinate()) {
            indicator_ = LoadingIndicator(LoadingIndicatorOptions{.progress = 0.0, .contained = true, .size = t().container_size});
        }
        turn_ = 0.0;
        spinner_mix_.snap_to(0.0);
        spinner_ms_ = 0.0;
        arrow_alpha_.snap_to(a().min_alpha);
    }
}

bool PullToRefresh::animating() const {
    if (!distance_.settled()) {
        return true;
    }
    if (!shown()) {
        return false;
    }
    if (options_.style == PullIndicatorStyle::arrow) {
        return spinning() || spinner_mix_.value() > 0.0 || !arrow_alpha_.settled();
    }
    return indicator_.animating();
}

void PullToRefresh::paint_arrow(render::Canvas& canvas, color::Argb color) const {
    const tokens::PullArrowTokens& k = a();
    const PullArrow a = arrow();
    const double size = k.container_size;
    const render::Point c{size / 2.0, size / 2.0};
    const double radius = k.arc_radius + k.stroke / 2.0;
    const double sweep = a.end_degrees - a.start_degrees;
    if (sweep > 0.0) {
        const int n = arc_segments(sweep, k.arc_segments_per_turn);
        render::Path arc;
        for (int i = 0; i <= n; ++i) {
            const render::Point p = on_circle(c, radius + k.stroke / 2.0, a.start_degrees + sweep * i / n);
            if (i == 0) {
                arc.move_to(p);
            } else {
                arc.line_to(p);
            }
        }
        for (int i = n; i >= 0; --i) {
            arc.line_to(on_circle(c, radius - k.stroke / 2.0, a.start_degrees + sweep * i / n));
        }
        arc.close();
        canvas.fill_path(arc, color);
    }
    if (a.scale > 0.0) {
        const double w = k.arrow_width * a.scale;
        const double h = k.arrow_height * a.scale;
        const double end = radians(a.end_degrees);
        const render::Point out{std::cos(end), std::sin(end)};
        const render::Point ahead{-std::sin(end), std::cos(end)};
        const render::Point base = on_circle(c, radius, a.end_degrees) - ahead * k.stroke;
        render::Path head;
        head.move_to(base - out * (w / 2.0));
        head.line_to(base + ahead * h);
        head.line_to(base + out * (w / 2.0));
        head.close();
        canvas.fill_path(head, color);
    }
}

void PullToRefresh::paint_spinner(render::Canvas& canvas, color::Argb color) const {
    const tokens::PullArrowTokens& k = a();
    const double size = k.container_size;
    const render::Point c{size / 2.0, size / 2.0};
    const double radius = (k.spinner_size - k.stroke) / 2.0;
    const double start = spinner_rotation_degrees();
    const double sweep = spinner_sweep_degrees();
    const int n = arc_segments(sweep, k.arc_segments_per_turn);
    std::vector<render::Point> line;
    line.reserve(static_cast<std::size_t>(n) + 1);
    for (int i = 0; i <= n; ++i) {
        line.push_back(on_circle(c, radius, start + sweep * i / n));
    }
    canvas.fill_path(render::stroke_path(line, k.stroke), color);
}

void PullToRefresh::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (distance_.value() <= 0.0) {
        return;
    }
    const render::Rect r = container_rect();
    if (r.y + r.height <= area_.y) {
        return;
    }
    const double size = container_size();
    canvas.save();
    canvas.push_clip(area_);
    if (options_.style == PullIndicatorStyle::arrow) {
        const tokens::PullArrowTokens& k = a();
        paint_surface(canvas, r, current_style().shape.radius(t().container_shape, size, size), scheme[k.container], current_style().elevation.levels[k.elevation_level],
                      scheme[t().shadow]);
        canvas.translate(r.x, r.y);
        const double mix = std::clamp(spinner_mix_.value(), 0.0, 1.0);
        if (mix < 1.0) {
            paint_arrow(canvas, color::scale_alpha(scheme[k.indicator], arrow_alpha_.value() * (1.0 - mix)));
        }
        if (mix > 0.0) {
            paint_spinner(canvas, color::scale_alpha(scheme[k.indicator], mix));
        }
        canvas.pop_clip();
        canvas.restore();
        return;
    }
    paint_surface(canvas, r, current_style().shape.radius(t().container_shape, size, size), scheme[t().container],
                  current_style().elevation.levels[t().elevation_level], scheme[t().shadow]);
    canvas.translate(r.x + size / 2.0, r.y + size / 2.0);
    canvas.rotate(turn_ * std::numbers::pi / 180.0);
    canvas.scale(indicator_scale());
    canvas.translate(-size / 2.0, -size / 2.0);
    canvas.fill_path(indicator_.indicator_path(), scheme[t().indicator]);
    canvas.pop_clip();
    canvas.restore();
}

}
