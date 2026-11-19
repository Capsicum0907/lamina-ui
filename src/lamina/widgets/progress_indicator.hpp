#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/progress_indicator.hpp"

namespace lamina::widgets {

struct ProgressColors {
    color::Role indicator = tokens::circular_progress_tokens.indicator;
    color::Role track = tokens::circular_progress_tokens.track;
};

struct CircularProgressOptions {
    double diameter = tokens::circular_progress_tokens.wavy_size;
    double thickness = tokens::circular_progress_tokens.active_thickness;
    double track_thickness = tokens::circular_progress_tokens.track_thickness;
    double gap = tokens::circular_progress_tokens.track_active_space;
    double amplitude = tokens::circular_progress_tokens.wave_amplitude;
    double wavelength = tokens::circular_progress_tokens.wavelength;
    double waves_per_second = tokens::circular_progress_tokens.waves_per_second;
    double sample_length = tokens::circular_progress_tokens.sample_length;
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
};

}
