#include "lamina/style/style.hpp"
#include "lamina/widgets/slider_track.hpp"

#include "lamina/tokens/typography.hpp"

namespace lamina::widgets {

namespace {

const tokens::SliderTokens& t() {
    return current_style().slider;
}

render::CornerRadii clamped(render::CornerRadii r, const render::Rect& rect) {
    const double cap = std::min(rect.width, rect.height) / 2.0;
    return {std::min(r.top_left, cap), std::min(r.top_right, cap), std::min(r.bottom_right, cap), std::min(r.bottom_left, cap)};
}

}

SliderPalette slider_palette(const color::ColorScheme& scheme, const Enablement& enabled) {
    auto of = [&](const tokens::StateColor& disabled, color::Role on) { return enabled.mix(resolve(scheme, disabled), scheme[on]); };
    return {of(t().disabled_active_track, t().active_track),   of(t().disabled_inactive_track, t().inactive_track),
            of(t().disabled_active_tick, t().active_tick),     of(t().disabled_inactive_tick, t().inactive_tick),
            of(t().disabled_stop_indicator, t().stop_indicator), of(t().disabled_handle, t().handle)};
}

void paint_track(render::Canvas& canvas, const SliderAxis& axis, double thickness, const std::vector<TrackSegment>& segments,
                 const SliderPalette& palette) {
    for (const TrackSegment& s : segments) {
        if (s.to <= s.from) {
            continue;
        }
        const render::Rect r = axis.rect(s.from, s.to, thickness);
        canvas.fill_path(render::rounded_rect_path(r, clamped(axis.radii(s.start_radius, s.end_radius), r)),
                         s.active ? palette.active_track : palette.inactive_track);
    }
}

void paint_stop(render::Canvas& canvas, const SliderAxis& axis, double along, const SliderPalette& palette) {
    canvas.fill_path(render::circle_path(axis.point(along), t().stop_indicator_size / 2.0), palette.stop_indicator);
}

void paint_ticks(render::Canvas& canvas, const SliderAxis& axis, const std::vector<double>& ticks, const std::vector<TrackSegment>& segments,
                 const std::vector<std::pair<double, double>>& hidden, const SliderPalette& palette) {
    for (double tick : ticks) {
        const bool covered = std::any_of(hidden.begin(), hidden.end(), [tick](const auto& h) { return tick >= h.first && tick <= h.second; });
        if (covered) {
            continue;
        }
        const bool active = std::any_of(segments.begin(), segments.end(),
                                        [tick](const TrackSegment& s) { return s.active && tick >= s.from && tick <= s.to; });
        canvas.fill_path(render::circle_path(axis.point(tick), t().tick_size / 2.0), active ? palette.active_tick : palette.inactive_tick);
    }
}

void paint_handle(render::Canvas& canvas, const render::Rect& handle, const SliderPalette& palette) {
    const double r = std::min(handle.width, handle.height) / 2.0;
    canvas.fill_path(render::rounded_rect_path(handle, render::uniform_radii(r)), palette.handle);
}

double thumb_along(double fraction, double length, double corner, int steps) {
    if (steps > 0 && fraction != 0.0 && fraction != 1.0) {
        return (length - 2.0 * corner) * fraction + corner;
    }
    return length * fraction;
}

std::vector<double> tick_alongs(int steps, double length, double corner) {
    std::vector<double> out;
    if (steps <= 0) {
        return out;
    }
    const int intervals = steps + 1;
    for (int i = 0; i <= intervals; ++i) {
        out.push_back(corner + (length - 2.0 * corner) * i / intervals);
    }
    return out;
}

double snap_value(double value, double min, double max, int steps) {
    const double lo = std::min(min, max);
    const double hi = std::max(min, max);
    const double v = std::clamp(value, lo, hi);
    if (steps <= 0) {
        return v;
    }
    const int intervals = steps + 1;
    double best = min;
    double best_distance = std::abs(v - best);
    for (int i = 1; i <= intervals; ++i) {
        const double tick = min + (max - min) * i / intervals;
        const double d = std::abs(v - tick);
        if (d < best_distance) {
            best = tick;
            best_distance = d;
        }
    }
    return best;
}

SliderKeyStep slider_key_step(double min, double max, int steps) {
    const double range = std::abs(max - min);
    const int intervals = steps > 0 ? steps + 1 : t().keyboard_steps_when_continuous;
    const double delta = range / intervals;
    return {delta, std::clamp(intervals / t().keyboard_page_divisor, 1, t().keyboard_page_max_steps) * delta};
}

ValueIndicator::ValueIndicator() : scale_(0.0) {}

void ValueIndicator::set_shown(bool shown) {
    if (shown == shown_) {
        return;
    }
    shown_ = shown;
    scale_.animate_to(shown ? 1.0 : 0.0, t().value_indicator_scale);
}

render::Rect ValueIndicator::rect(const text::FontStack& fonts, const std::string& text, const render::Rect& handle) const {
    const text::TextLine line = text::layout_line(fonts, text, tokens::text_style(tokens::type_style(t().value_indicator_type)));
    const double h = std::max(t().value_indicator_min_size, line.box_height() + 2.0 * t().value_indicator_padding);
    const double w = std::max(h, line.width + 2.0 * t().value_indicator_padding);
    const double cx = handle.x + handle.width / 2.0;
    return {cx - w / 2.0, handle.y - t().value_indicator_bottom_space - h, w, h};
}

void ValueIndicator::paint(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts, const std::string& text,
                           const render::Rect& handle) const {
    const double s = std::clamp(scale_.value(), 0.0, 1.0);
    if (s <= 0.0 || text.empty()) {
        return;
    }
    const text::TextLine line = text::layout_line(fonts, text, tokens::text_style(tokens::type_style(t().value_indicator_type)));
    const render::Rect r = rect(fonts, text, handle);
    canvas.save();
    canvas.translate(r.x + r.width / 2.0, r.y + r.height);
    canvas.scale(s);
    const render::Rect local{-r.width / 2.0, -r.height, r.width, r.height};
    canvas.fill_path(render::rounded_rect_path(local, render::uniform_radii(r.height / 2.0)), scheme[t().value_indicator_container]);
    text::draw_line(canvas, fonts, line, {-line.width / 2.0, -r.height + (r.height - line.box_height()) / 2.0},
                    scheme[t().value_indicator_label]);
    canvas.restore();
}

}
