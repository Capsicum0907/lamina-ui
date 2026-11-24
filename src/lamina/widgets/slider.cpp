#include "lamina/style/style.hpp"
#include "lamina/widgets/slider.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

const tokens::SliderTokens& t() {
    return current_style().slider;
}

double end_gap() {
    return t().handle_width / 2.0 + t().handle_track_gap;
}

}

Slider::Slider(SliderOptions options) : options_(std::move(options)), enabled_(options_.enabled) {
    options_.value = snap_value(options_.value, options_.min, options_.max, options_.steps);
}

Slider::Slider(SliderOptions options, const text::FontStack& fonts) : Slider(std::move(options)) {
    fonts_ = fonts;
}

double Slider::width() const {
    return vertical() ? t().handle_height : length_;
}

double Slider::height() const {
    return vertical() ? length_ : t().handle_height;
}

void Slider::set_width(double width) {
    if (!vertical()) {
        length_ = width;
    }
}

layout::Size Slider::measure(const layout::Constraints& c) const {
    if (vertical()) {
        return {t().handle_height, std::isfinite(c.max_height) ? c.max_height : length_};
    }
    return {std::isfinite(c.max_width) ? c.max_width : length_, t().handle_height};
}

void Slider::arrange(const render::Rect& r) {
    if (vertical()) {
        length_ = r.height;
        origin_ = {r.x + (r.width - t().handle_height) / 2.0, r.y};
    } else {
        length_ = r.width;
        origin_ = {r.x, r.y + (r.height - t().handle_height) / 2.0};
    }
}

render::Rect Slider::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

SliderAxis Slider::axis() const {
    return {bounds(), vertical(), vertical() && !options_.top_to_bottom, t().handle_width};
}

double Slider::corner() const {
    return options_.track_corner.value_or(t().track_height / 2.0);
}

void Slider::set_value(double value) {
    options_.value = snap_value(value, options_.min, options_.max, options_.steps);
}

double Slider::fraction() const {
    const double range = options_.max - options_.min;
    if (range <= 0.0) {
        return 0.0;
    }
    return std::clamp((options_.value - options_.min) / range, 0.0, 1.0);
}

double Slider::value_along() const {
    return thumb_along(fraction(), axis().length(), corner(), options_.steps);
}

double Slider::handle_width() const {
    return narrowed_ ? t().pressed_handle_width : t().handle_width;
}

render::Point Slider::handle_center() const {
    return axis().point(value_along());
}

double Slider::handle_center_x() const {
    return handle_center().x;
}

render::Rect Slider::handle_rect() const {
    const render::Point c = handle_center();
    const double along = handle_width();
    if (vertical()) {
        return {c.x - t().handle_height / 2.0, c.y - along / 2.0, t().handle_height, along};
    }
    return {c.x - along / 2.0, c.y - t().handle_height / 2.0, along, t().handle_height};
}

std::vector<double> Slider::tick_centers() const {
    std::vector<double> out;
    const SliderAxis a = axis();
    for (double along : tick_alongs(options_.steps, a.length(), corner())) {
        const render::Point p = a.point(along);
        out.push_back(vertical() ? p.y : p.x);
    }
    return out;
}

std::vector<TrackSegment> Slider::segments() const {
    const double l = axis().length();
    const double c = corner();
    const double inside = t().track_inside_corner;
    const double v = value_along();
    const double gap = end_gap();
    const bool shrinking = (options_.track_corner.has_value() || vertical() || options_.centered) && options_.steps == 0;
    const double threshold = shrinking ? 0.0 : c;
    std::vector<TrackSegment> out;
    if (!options_.centered) {
        if (v - gap > threshold) {
            out.push_back({0.0, v - gap, c, inside, true});
        }
        if (v < l - gap - threshold) {
            out.push_back({v + gap, l, inside, c, false});
        }
        return out;
    }
    const double mid = l / 2.0;
    const double side_gap = t().handle_track_gap;
    const double active_threshold = shrinking ? 0.0 : inside;
    double left_end = 0.0;
    double right_start = 0.0;
    if (v > mid) {
        left_end = mid - side_gap;
        right_start = v + gap;
        if (v - gap - mid > active_threshold) {
            out.push_back({mid, v - gap, inside, inside, true});
        }
    } else if (v < mid) {
        left_end = v - gap;
        right_start = mid + side_gap;
        if (mid - (v + gap) > active_threshold) {
            out.push_back({v + gap, mid, inside, inside, true});
        }
    } else {
        left_end = v - gap;
        right_start = v + gap;
    }
    if (left_end > threshold) {
        out.push_back({0.0, left_end, c, inside, false});
    }
    if (right_start < l - threshold) {
        out.push_back({right_start, l, inside, c, false});
    }
    return out;
}

double Slider::value_at(double x, double y) const {
    const double f = axis().fraction_at(x, y);
    return snap_value(options_.min + (options_.max - options_.min) * f, options_.min, options_.max, options_.steps);
}

std::string Slider::value_text() const {
    return options_.value_label ? options_.value_label(options_.value) : std::string();
}

AccessibleNode Slider::accessible() const {
    return {AccessibleRole::slider, options_.accessible_name, bounds(),
            {.enabled = enabled(), .focusable = enabled(), .focused = focused(),
             .range = AccessibleRange{options_.value, options_.min, options_.max}},
            value_text()};
}

void Slider::update_handle() {
    narrowed_ = dragging_ || focus_.focused();
    indicator_.set_shown(static_cast<bool>(options_.value_label) && fonts_ && !vertical() && enabled() && narrowed_);
}

void Slider::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    update_handle();
}

void Slider::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        dragging_ = false;
        focus_.set_focused(false);
    }
    update_handle();
}

std::optional<TargetHit> Slider::target_hit(const input::PointerEvent& e) const {
    return widgets::target_hit(bounds(), e.x_dp, e.y_dp);
}

bool Slider::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (!enabled()) {
        update_handle();
        return false;
    }
    const double before = options_.value;
    switch (e.action) {
    case input::PointerAction::down:
        if (contains(hit_area(e, bounds()), e.x_dp, e.y_dp)) {
            dragging_ = true;
            options_.value = value_at(e.x_dp, e.y_dp);
        }
        break;
    case input::PointerAction::move:
        if (dragging_) {
            options_.value = value_at(e.x_dp, e.y_dp);
        }
        break;
    case input::PointerAction::up:
        if (dragging_) {
            options_.value = value_at(e.x_dp, e.y_dp);
        }
        dragging_ = false;
        break;
    case input::PointerAction::leave:
        dragging_ = false;
        break;
    }
    update_handle();
    return options_.value != before;
}

void Slider::cancel_interaction() {
    dragging_ = false;
    update_handle();
}

bool Slider::handle(const input::KeyEvent& e) {
    if (!enabled() || !focus_.focused()) {
        return false;
    }
    const SliderKeyStep k = slider_key_step(options_.min, options_.max, options_.steps);
    const double before = options_.value;
    if (vertical()) {
        const double sign = options_.top_to_bottom ? 1.0 : -1.0;
        switch (e.key) {
        case input::Key::up: set_value(options_.value - sign * k.delta); break;
        case input::Key::down: set_value(options_.value + sign * k.delta); break;
        case input::Key::page_up: set_value(options_.value - sign * k.page); break;
        case input::Key::page_down: set_value(options_.value + sign * k.page); break;
        case input::Key::home: set_value(options_.min); break;
        case input::Key::end: set_value(options_.max); break;
        default: return false;
        }
        return options_.value != before;
    }
    switch (e.key) {
    case input::Key::left:
    case input::Key::down: set_value(options_.value - k.delta); break;
    case input::Key::right:
    case input::Key::up: set_value(options_.value + k.delta); break;
    case input::Key::page_down: set_value(options_.value - k.page); break;
    case input::Key::page_up: set_value(options_.value + k.page); break;
    case input::Key::home: set_value(options_.min); break;
    case input::Key::end: set_value(options_.max); break;
    default: return false;
    }
    return options_.value != before;
}

void Slider::step(double dt) {
    focus_.step(dt);
    enabled_.step(dt);
    indicator_.step(dt);
}

bool Slider::animating() const {
    return !focus_.settled() || !enabled_.settled() || !indicator_.settled();
}

void Slider::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const SliderAxis a = axis();
    const SliderPalette palette = slider_palette(scheme, enabled_);
    const std::vector<TrackSegment> segs = segments();
    paint_track(canvas, a, t().track_height, segs, palette);
    const double l = a.length();
    const double c = corner();
    const bool start_shown = std::any_of(segs.begin(), segs.end(), [](const TrackSegment& s) { return !s.active && s.from == 0.0; });
    const bool end_shown = std::any_of(segs.begin(), segs.end(), [l](const TrackSegment& s) { return !s.active && s.to == l; });
    if (end_shown) {
        paint_stop(canvas, a, l - c, palette);
    }
    if (options_.centered && start_shown) {
        paint_stop(canvas, a, c, palette);
    }
    std::vector<double> ticks = tick_alongs(options_.steps, l, c);
    if (!ticks.empty()) {
        ticks.pop_back();
        if (options_.centered) {
            ticks.erase(ticks.begin());
        }
    }
    const double v = value_along();
    std::vector<std::pair<double, double>> hidden{{v - end_gap(), v + end_gap()}};
    if (options_.centered) {
        hidden.push_back({l / 2.0 - end_gap(), l / 2.0 + end_gap()});
    }
    paint_ticks(canvas, a, ticks, segs, hidden, palette);
    const render::Rect handle = handle_rect();
    paint_handle(canvas, handle, palette);
    focus_.paint(canvas, scheme, handle, std::min(handle.width, handle.height) / 2.0);
    if (fonts_ && options_.value_label) {
        indicator_.paint(canvas, scheme, *fonts_, value_text(), handle);
    }
}

}
