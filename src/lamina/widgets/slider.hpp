#pragma once

#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/slider.hpp"

namespace lamina::widgets {

struct SliderOptions {
    double value = 0.0;
    double min = 0.0;
    double max = 1.0;
    int steps = 0;
};

class Slider {
public:
    explicit Slider(SliderOptions options = {});

    double width() const { return width_; }
    double height() const;
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    double value() const { return options_.value; }
    void set_value(double value);
    double fraction() const;
    bool dragging() const { return dragging_; }
    double handle_width() const { return handle_width_.value(); }
    double handle_center_x() const;
    std::vector<double> tick_centers() const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double track_x() const;
    double track_width() const;
    double value_end_in_track() const;
    double snapped(double value) const;
    double value_at(double x) const;
    bool on_first_or_last_tick() const;

    SliderOptions options_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool dragging_ = false;
    motion::Animated handle_width_;
};

}
