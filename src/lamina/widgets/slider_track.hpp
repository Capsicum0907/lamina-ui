#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/slider.hpp"
#include "lamina/widgets/enablement.hpp"

namespace lamina::widgets {

enum class SliderOrientation { horizontal, vertical };

struct SliderAxis {
    render::Rect bounds;
    bool vertical = false;
    bool reversed = false;
    double thumb_extent = 0.0;

    double length() const { return std::max(0.0, (vertical ? bounds.height : bounds.width) - thumb_extent); }
    double cross_center() const { return vertical ? bounds.x + bounds.width / 2.0 : bounds.y + bounds.height / 2.0; }
    double along_start() const { return (vertical ? bounds.y : bounds.x) + thumb_extent / 2.0; }

    double screen(double along) const {
        return reversed ? along_start() + length() - along : along_start() + along;
    }
    render::Point point(double along) const {
        const double s = screen(along);
        return vertical ? render::Point{cross_center(), s} : render::Point{s, cross_center()};
    }
    render::Rect rect(double from, double to, double thickness) const {
        const double a = std::min(screen(from), screen(to));
        const double b = std::max(screen(from), screen(to));
        const double c = cross_center() - thickness / 2.0;
        return vertical ? render::Rect{c, a, thickness, b - a} : render::Rect{a, c, b - a, thickness};
    }
    render::CornerRadii radii(double start, double end) const {
        const double lo = reversed ? end : start;
        const double hi = reversed ? start : end;
        if (vertical) {
            return {lo, lo, hi, hi};
        }
        return {lo, hi, hi, lo};
    }
    double along_of(double x, double y) const {
        const double s = (vertical ? y : x) - along_start();
        return reversed ? length() - s : s;
    }
    double fraction_at(double x, double y) const {
        const double l = length();
        return l > 0.0 ? std::clamp(along_of(x, y) / l, 0.0, 1.0) : 0.0;
    }
};

struct TrackSegment {
    double from;
    double to;
    double start_radius;
    double end_radius;
    bool active;
};

struct SliderPalette {
    color::Argb active_track;
    color::Argb inactive_track;
    color::Argb active_tick;
    color::Argb inactive_tick;
    color::Argb stop_indicator;
    color::Argb handle;
};

SliderPalette slider_palette(const color::ColorScheme& scheme, const Enablement& enabled);

void paint_track(render::Canvas& canvas, const SliderAxis& axis, double thickness, const std::vector<TrackSegment>& segments,
                 const SliderPalette& palette);
void paint_stop(render::Canvas& canvas, const SliderAxis& axis, double along, const SliderPalette& palette);
void paint_ticks(render::Canvas& canvas, const SliderAxis& axis, const std::vector<double>& ticks, const std::vector<TrackSegment>& segments,
                 const std::vector<std::pair<double, double>>& hidden, const SliderPalette& palette);
void paint_handle(render::Canvas& canvas, const render::Rect& handle, const SliderPalette& palette);

double thumb_along(double fraction, double length, double corner, int steps);
std::vector<double> tick_alongs(int steps, double length, double corner);
double snap_value(double value, double min, double max, int steps);

struct SliderKeyStep {
    double delta;
    double page;
};

SliderKeyStep slider_key_step(double min, double max, int steps);

class ValueIndicator {
public:
    ValueIndicator();

    void set_shown(bool shown);
    bool shown() const { return shown_; }
    double scale() const { return scale_.value(); }
    void step(double dt_seconds) { scale_.step(dt_seconds); }
    bool settled() const { return scale_.settled(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts, const std::string& text,
               const render::Rect& handle) const;
    render::Rect rect(const text::FontStack& fonts, const std::string& text, const render::Rect& handle) const;

private:
    bool shown_ = false;
    motion::Tweened scale_;
};

}
