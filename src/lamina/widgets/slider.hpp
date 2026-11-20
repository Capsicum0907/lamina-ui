#pragma once

#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include <string>

#include "lamina/tokens/slider.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"

namespace lamina::widgets {

struct SliderOptions {
    double value = 0.0;
    double min = 0.0;
    double max = 1.0;
    int steps = 0;
    bool enabled = true;
    std::string accessible_name = {};
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
    double handle_width() const { return handle_width_; }
    double handle_center_x() const;
    std::vector<double> tick_centers() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const {
        return {AccessibleRole::slider, options_.accessible_name, bounds(),
                {.enabled = enabled(), .focusable = enabled(), .focused = focused(),
                 .range = AccessibleRange{options_.value, options_.min, options_.max}}};
    }

    bool handle(const input::PointerEvent& event);
    void cancel_interaction();
    bool handle(const input::KeyEvent& event);
    void step(double dt_seconds) {
        focus_.step(dt_seconds);
        enabled_.step(dt_seconds);
    }
    bool animating() const { return !focus_.settled() || !enabled_.settled(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double track_x() const;
    double track_width() const;
    double value_end_in_track() const;
    double snapped(double value) const;
    double value_at(double x) const;
    bool on_first_or_last_tick() const;
    void update_handle_width();

    SliderOptions options_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool dragging_ = false;
    double handle_width_ = tokens::slider_tokens.handle_width;
    FocusIndicator focus_;
    Enablement enabled_;
};

}
