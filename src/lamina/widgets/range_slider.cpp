#include "lamina/widgets/range_slider.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SliderTokens& t = tokens::slider_tokens;

double end_gap() {
    return t.handle_width / 2.0 + t.handle_track_gap;
}

}

bool RangeSliderThumb::focused() const {
    return slider_.focused(which_);
}

void RangeSliderThumb::set_focused(bool focused) {
    slider_.set_focused(which_, focused);
}

bool RangeSliderThumb::can_focus() const {
    return slider_.can_focus();
}

render::Rect RangeSliderThumb::bounds() const {
    return slider_.handle_rect(which_);
}

RangeSlider::RangeSlider(RangeSliderOptions options)
    : options_(std::move(options)), thumbs_{RangeSliderThumb(*this, RangeThumb::start), RangeSliderThumb(*this, RangeThumb::end)},
      enabled_(options_.enabled) {
    const double a = std::clamp(std::min(options_.start, options_.end), std::min(options_.min, options_.max), std::max(options_.min, options_.max));
    const double b = std::clamp(std::max(options_.start, options_.end), std::min(options_.min, options_.max), std::max(options_.min, options_.max));
    values_ = {snap_value(a, options_.min, options_.max, options_.steps), snap_value(b, options_.min, options_.max, options_.steps)};
}

RangeSlider::RangeSlider(RangeSliderOptions options, const text::FontStack& fonts) : RangeSlider(std::move(options)) {
    fonts_ = fonts;
}

double RangeSlider::height() const {
    return t.handle_height;
}

layout::Size RangeSlider::measure(const layout::Constraints& c) const {
    return {std::isfinite(c.max_width) ? c.max_width : width_, t.handle_height};
}

void RangeSlider::arrange(const render::Rect& r) {
    width_ = r.width;
    origin_ = {r.x, r.y + (r.height - t.handle_height) / 2.0};
}

render::Rect RangeSlider::bounds() const {
    return {origin_.x, origin_.y, width_, t.handle_height};
}

SliderAxis RangeSlider::axis() const {
    return {bounds(), false, false, t.handle_width};
}

double RangeSlider::corner() const {
    return t.track_height / 2.0;
}

double RangeSlider::fraction(double v) const {
    const double range = options_.max - options_.min;
    return range <= 0.0 ? 0.0 : std::clamp((v - options_.min) / range, 0.0, 1.0);
}

double RangeSlider::along(RangeThumb thumb) const {
    return thumb_along(fraction(value(thumb)), axis().length(), corner(), options_.steps);
}

double RangeSlider::value_at(double x) const {
    const double f = axis().fraction_at(x, 0.0);
    return options_.min + (options_.max - options_.min) * f;
}

void RangeSlider::set_range(double start, double end) {
    values_[0] = snap_value(std::min(start, end), options_.min, options_.max, options_.steps);
    values_[1] = snap_value(std::max(start, end), options_.min, options_.max, options_.steps);
}

void RangeSlider::move(RangeThumb thumb, double value) {
    const double snapped = snap_value(value, options_.min, options_.max, options_.steps);
    if (thumb == RangeThumb::start) {
        values_[0] = std::min(snapped, values_[1]);
    } else {
        values_[1] = std::max(snapped, values_[0]);
    }
}

double RangeSlider::handle_width(RangeThumb thumb) const {
    const bool narrowed = dragging_ == thumb || focused(thumb);
    return narrowed ? t.pressed_handle_width : t.handle_width;
}

render::Rect RangeSlider::handle_rect(RangeThumb thumb) const {
    const render::Point c = axis().point(along(thumb));
    const double w = handle_width(thumb);
    return {c.x - w / 2.0, c.y - t.handle_height / 2.0, w, t.handle_height};
}

std::vector<TrackSegment> RangeSlider::segments() const {
    const double l = axis().length();
    const double c = corner();
    const double inside = t.track_inside_corner;
    const double vs = along(RangeThumb::start);
    const double ve = along(RangeThumb::end);
    const double gap = end_gap();
    std::vector<TrackSegment> out;
    if (vs > gap + c) {
        out.push_back({0.0, vs - gap, c, inside, false});
    }
    if ((ve - gap) - (vs + gap) > inside) {
        out.push_back({vs + gap, ve - gap, inside, inside, true});
    }
    if (ve < l - gap - c) {
        out.push_back({ve + gap, l, inside, c, false});
    }
    return out;
}

std::vector<double> RangeSlider::tick_centers() const {
    std::vector<double> out;
    for (double a : tick_alongs(options_.steps, axis().length(), corner())) {
        out.push_back(axis().point(a).x);
    }
    return out;
}

std::string RangeSlider::value_text(RangeThumb thumb) const {
    return options_.value_label ? options_.value_label(value(thumb)) : std::string();
}

std::optional<RangeThumb> RangeSlider::focused_thumb() const {
    if (focus_[0].focused()) {
        return RangeThumb::start;
    }
    if (focus_[1].focused()) {
        return RangeThumb::end;
    }
    return std::nullopt;
}

void RangeSlider::set_focused(RangeThumb thumb, bool focused) {
    focus_[index(thumb)].set_focused(focused && enabled());
    update_handles();
}

void RangeSlider::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        dragging_.reset();
        focus_[0].set_focused(false);
        focus_[1].set_focused(false);
    }
    update_handles();
}

AccessibleNode RangeSlider::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name, bounds(), {.enabled = enabled()}};
    node.children.push_back({AccessibleRole::slider, options_.names.start, handle_rect(RangeThumb::start),
                             {.enabled = enabled(), .focusable = enabled(), .focused = focused(RangeThumb::start),
                              .range = AccessibleRange{values_[0], options_.min, values_[1]}},
                             value_text(RangeThumb::start)});
    node.children.push_back({AccessibleRole::slider, options_.names.end, handle_rect(RangeThumb::end),
                             {.enabled = enabled(), .focusable = enabled(), .focused = focused(RangeThumb::end),
                              .range = AccessibleRange{values_[1], values_[0], options_.max}},
                             value_text(RangeThumb::end)});
    return node;
}

void RangeSlider::update_handles() {
    for (RangeThumb thumb : {RangeThumb::start, RangeThumb::end}) {
        const bool narrowed = dragging_ == thumb || focused(thumb);
        indicators_[index(thumb)].set_shown(static_cast<bool>(options_.value_label) && fonts_ && enabled() && narrowed);
    }
}

std::optional<TargetHit> RangeSlider::target_hit(const input::PointerEvent& e) const {
    return widgets::target_hit(bounds(), e.x_dp, e.y_dp);
}

bool RangeSlider::handle(const input::PointerEvent& e) {
    for (FocusIndicator& f : focus_) {
        f.release_on_press(e);
    }
    if (!enabled()) {
        update_handles();
        return false;
    }
    const std::array<double, 2> before = values_;
    switch (e.action) {
    case input::PointerAction::down:
        if (contains(hit_area(e, bounds()), e.x_dp, e.y_dp)) {
            const double ps = axis().point(along(RangeThumb::start)).x;
            const double pe = axis().point(along(RangeThumb::end)).x;
            const double ds = std::abs(ps - e.x_dp);
            const double de = std::abs(pe - e.x_dp);
            press_x_ = e.x_dp;
            choosing_by_direction_ = std::abs(ps - pe) < t.handle_width && ds < t.handle_width;
            const bool start = ds != de ? ds < de : ps > e.x_dp;
            dragging_ = start ? RangeThumb::start : RangeThumb::end;
            if (!choosing_by_direction_) {
                move(*dragging_, value_at(e.x_dp));
            }
        }
        break;
    case input::PointerAction::move:
        if (dragging_) {
            if (choosing_by_direction_ && e.x_dp != press_x_) {
                dragging_ = e.x_dp < press_x_ ? RangeThumb::start : RangeThumb::end;
                choosing_by_direction_ = false;
            }
            if (!choosing_by_direction_) {
                move(*dragging_, value_at(e.x_dp));
            }
        }
        break;
    case input::PointerAction::up:
        if (dragging_) {
            move(*dragging_, value_at(e.x_dp));
        }
        dragging_.reset();
        choosing_by_direction_ = false;
        break;
    case input::PointerAction::leave:
        dragging_.reset();
        choosing_by_direction_ = false;
        break;
    }
    update_handles();
    return values_ != before;
}

bool RangeSlider::handle(const input::KeyEvent& e) {
    const std::optional<RangeThumb> thumb = focused_thumb();
    if (!enabled() || !thumb) {
        return false;
    }
    const SliderKeyStep k = slider_key_step(options_.min, options_.max, options_.steps);
    const double v = value(*thumb);
    const bool start = *thumb == RangeThumb::start;
    const std::array<double, 2> before = values_;
    switch (e.key) {
    case input::Key::left:
    case input::Key::down: move(*thumb, v - k.delta); break;
    case input::Key::right:
    case input::Key::up: move(*thumb, v + k.delta); break;
    case input::Key::page_down: move(*thumb, v - k.page); break;
    case input::Key::page_up: move(*thumb, v + k.page); break;
    case input::Key::home: move(*thumb, start ? options_.min : values_[0]); break;
    case input::Key::end: move(*thumb, start ? values_[1] : options_.max); break;
    default: return false;
    }
    return values_ != before;
}

void RangeSlider::cancel_interaction() {
    dragging_.reset();
    choosing_by_direction_ = false;
    update_handles();
}

void RangeSlider::step(double dt) {
    for (FocusIndicator& f : focus_) {
        f.step(dt);
    }
    for (ValueIndicator& i : indicators_) {
        i.step(dt);
    }
    enabled_.step(dt);
}

bool RangeSlider::animating() const {
    return !focus_[0].settled() || !focus_[1].settled() || !indicators_[0].settled() || !indicators_[1].settled() || !enabled_.settled();
}

void RangeSlider::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const SliderAxis a = axis();
    const SliderPalette palette = slider_palette(scheme, enabled_);
    const std::vector<TrackSegment> segs = segments();
    paint_track(canvas, a, t.track_height, segs, palette);
    const double l = a.length();
    const double c = corner();
    for (const TrackSegment& s : segs) {
        if (!s.active && s.from == 0.0) {
            paint_stop(canvas, a, c, palette);
        }
        if (!s.active && s.to == l) {
            paint_stop(canvas, a, l - c, palette);
        }
    }
    std::vector<double> ticks = tick_alongs(options_.steps, l, c);
    if (ticks.size() >= 2) {
        ticks.pop_back();
        ticks.erase(ticks.begin());
    }
    const double vs = along(RangeThumb::start);
    const double ve = along(RangeThumb::end);
    paint_ticks(canvas, a, ticks, segs, {{vs - end_gap(), vs + end_gap()}, {ve - end_gap(), ve + end_gap()}}, palette);
    for (RangeThumb thumb : {RangeThumb::start, RangeThumb::end}) {
        const render::Rect h = handle_rect(thumb);
        paint_handle(canvas, h, palette);
        focus_[index(thumb)].paint(canvas, scheme, h, h.width / 2.0);
    }
    if (fonts_ && options_.value_label) {
        for (RangeThumb thumb : {RangeThumb::start, RangeThumb::end}) {
            indicators_[index(thumb)].paint(canvas, scheme, *fonts_, value_text(thumb), handle_rect(thumb));
        }
    }
}

}
