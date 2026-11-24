#pragma once

#include <string>

#include "lamina/style/style.hpp"
#include "lamina/widgets/accessibility.hpp"

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/progress_indicator.hpp"

namespace lamina::widgets {

struct ProgressColors {
    color::Role indicator = current_style().circular_progress.indicator;
    color::Role track = current_style().circular_progress.track;
};

struct CircularProgressOptions {
    double diameter = current_style().circular_progress.wavy_size;
    double thickness = current_style().circular_progress.active_thickness;
    double track_thickness = current_style().circular_progress.track_thickness;
    double gap = current_style().circular_progress.track_active_space;
    double amplitude = current_style().circular_progress.wave_amplitude;
    double wavelength = current_style().circular_progress.wavelength;
    double waves_per_second = current_style().circular_progress.waves_per_second;
    double sample_length = current_style().circular_progress.sample_length;
    bool wavy = true;
    ProgressColors colors = {};
};

class CircularProgressIndicator {
public:
    explicit CircularProgressIndicator(CircularProgressOptions options = {});

    double width() const { return width_; }
    double height() const { return height_; }
    void set_size(double width, double height);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height_}; }
    double diameter() const;

    double progress() const;
    void set_progress(double progress);
    void ease_progress(double progress);
    void set_wavy(bool wavy);
    void set_colors(ProgressColors colors);
    double amplitude() const { return amplitude_.value(); }

    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void set_accessible_name(std::string name) { accessible_name_ = std::move(name); }
    AccessibleNode accessible() const {
        return {AccessibleRole::progress_indicator, accessible_name_, bounds(), {.range = AccessibleRange{progress(), 0.0, 1.0}}};
    }

private:
    double radius() const;
    double target_amplitude() const;
    void stroke_arc(render::Canvas& canvas, double from, double to, double thickness, double amplitude,
                    color::Argb color) const;

    CircularProgressOptions options_;
    ProgressColors previous_colors_;
    double width_;
    double height_;
    render::Point origin_{0.0, 0.0};
    double target_ = 0.0;
    double phase_ = 0.0;
    motion::Animated offset_;
    motion::Animated amplitude_;
    motion::Animated color_mix_;
    std::string accessible_name_;
};

}
