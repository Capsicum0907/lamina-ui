#include "lamina/widgets/slider.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SliderTokens& t = tokens::slider_tokens;

render::Path track_path(const render::Rect& r, double start_radius, double end_radius) {
    return render::rounded_rect_path(r, {start_radius, end_radius, end_radius, start_radius});
}

}

Slider::Slider(SliderOptions options) : options_(options) {
    options_.value = snapped(options_.value);
}

double Slider::height() const {
    return t.handle_height;
}

render::Rect Slider::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

void Slider::set_value(double value) {
    options_.value = snapped(value);
}

double Slider::fraction() const {
    const double range = options_.max - options_.min;
    if (range <= 0.0) {
        return 0.0;
    }
    return std::clamp((options_.value - options_.min) / range, 0.0, 1.0);
}

double Slider::snapped(double value) const {
    const double lo = std::min(options_.min, options_.max);
    const double hi = std::max(options_.min, options_.max);
    const double v = std::clamp(value, lo, hi);
    if (options_.steps <= 0) {
        return v;
    }
    const int intervals = options_.steps + 1;
    double best = options_.min;
    double best_distance = std::abs(v - best);
    for (int i = 1; i <= intervals; ++i) {
        const double tick = options_.min + (options_.max - options_.min) * i / intervals;
        const double d = std::abs(v - tick);
        if (d < best_distance) {
            best = tick;
            best_distance = d;
        }
    }
    return best;
}

double Slider::track_x() const {
    return origin_.x + t.handle_width / 2.0;
}

double Slider::track_width() const {
    return std::max(0.0, width_ - t.handle_width);
}

bool Slider::on_first_or_last_tick() const {
    const double f = fraction();
    return f == 0.0 || f == 1.0;
}

double Slider::value_end_in_track() const {
    const double corner = t.track_height / 2.0;
    const double f = fraction();
    if (options_.steps > 0 && !on_first_or_last_tick()) {
        return (track_width() - 2.0 * corner) * f + corner;
    }
    return track_width() * f;
}

double Slider::handle_center_x() const {
    return track_x() + value_end_in_track();
}

std::vector<double> Slider::tick_centers() const {
    std::vector<double> out;
    if (options_.steps <= 0) {
        return out;
    }
    const double corner = t.track_height / 2.0;
    const double start = track_x() + corner;
    const double end = track_x() + track_width() - corner;
    const int intervals = options_.steps + 1;
    for (int i = 0; i <= intervals; ++i) {
        out.push_back(start + (end - start) * i / intervals);
    }
    return out;
}

double Slider::value_at(double x) const {
    const double max_px = std::max(width_ - t.handle_width / 2.0, 0.0);
    const double min_px = std::min(t.handle_width / 2.0, max_px);
    const double offset = std::clamp(x - origin_.x, min_px, max_px);
    const double f = max_px > min_px ? (offset - min_px) / (max_px - min_px) : 0.0;
    return snapped(options_.min + (options_.max - options_.min) * f);
}

void Slider::update_handle_width() {
    handle_width_ = dragging_ || focus_.focused() ? t.pressed_handle_width : t.handle_width;
}

void Slider::set_focused(bool focused) {
    focus_.set_focused(focused);
    update_handle_width();
}

bool Slider::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const double before = options_.value;
    switch (e.action) {
    case input::PointerAction::down:
        if (contains(touch_target(bounds()), e.x_dp, e.y_dp)) {
            dragging_ = true;
            options_.value = value_at(e.x_dp);
        }
        break;
    case input::PointerAction::move:
        if (dragging_) {
            options_.value = value_at(e.x_dp);
        }
        break;
    case input::PointerAction::up:
        if (dragging_) {
            options_.value = value_at(e.x_dp);
        }
        dragging_ = false;
        break;
    case input::PointerAction::leave:
        dragging_ = false;
        break;
    }
    update_handle_width();
    return options_.value != before;
}

void Slider::cancel_interaction() {
    dragging_ = false;
    update_handle_width();
}

bool Slider::handle(const input::KeyEvent& e) {
    if (!focus_.focused()) {
        return false;
    }
    const double range = std::abs(options_.max - options_.min);
    const int intervals = options_.steps > 0 ? options_.steps + 1 : t.keyboard_steps_when_continuous;
    const double delta = range / intervals;
    const double page = std::clamp(intervals / t.keyboard_page_divisor, 1, t.keyboard_page_max_steps) * delta;
    const double before = options_.value;
    switch (e.key) {
    case input::Key::left:
    case input::Key::down:
        set_value(options_.value - delta);
        break;
    case input::Key::right:
    case input::Key::up:
        set_value(options_.value + delta);
        break;
    case input::Key::page_down:
        set_value(options_.value - page);
        break;
    case input::Key::page_up:
        set_value(options_.value + page);
        break;
    case input::Key::home:
        set_value(options_.min);
        break;
    case input::Key::end:
        set_value(options_.max);
        break;
    default:
        return false;
    }
    return options_.value != before;
}

void Slider::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double corner = t.track_height / 2.0;
    const double inside = t.track_inside_corner;
    const double x0 = track_x();
    const double tw = track_width();
    const double cy = origin_.y + height() / 2.0;
    const double top = cy - t.track_height / 2.0;
    const double value_end = value_end_in_track();
    const double end_gap = t.handle_width / 2.0 + t.handle_track_gap;

    const double inactive_threshold = tw - end_gap - corner;
    if (value_end < inactive_threshold) {
        const double start = value_end + end_gap;
        canvas.fill_path(track_path({x0 + start, top, tw - start, t.track_height}, inside, corner),
                         scheme[t.inactive_track]);
        canvas.fill_path(render::circle_path({x0 + tw - corner, cy}, t.stop_indicator_size / 2.0), scheme[t.stop_indicator]);
    }
    const double active_end = value_end - end_gap;
    if (active_end > corner) {
        canvas.fill_path(track_path({x0, top, active_end, t.track_height}, corner, inside), scheme[t.active_track]);
    }

    const std::vector<double> ticks = tick_centers();
    for (std::size_t i = 0; i + 1 < ticks.size(); ++i) {
        const double local = ticks[i] - x0;
        if (local >= value_end - end_gap && local <= value_end + end_gap) {
            continue;
        }
        const bool active = local >= 0.0 && local <= active_end;
        canvas.fill_path(render::circle_path({ticks[i], cy}, t.tick_size / 2.0), scheme[active ? t.active_tick : t.inactive_tick]);
    }

    const double w = handle_width_;
    const render::Rect handle{handle_center_x() - w / 2.0, origin_.y, w, height()};
    canvas.fill_path(render::rounded_rect_path(handle, render::uniform_radii(w / 2.0)), scheme[t.handle]);
    focus_.paint(canvas, scheme, handle, w / 2.0);
}

}
