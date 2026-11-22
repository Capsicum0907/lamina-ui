#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/slider.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/slider_track.hpp"

namespace lamina::widgets {

using SliderValueLabel = std::function<std::string(double)>;

struct SliderOptions {
    double value = 0.0;
    double min = 0.0;
    double max = 1.0;
    int steps = 0;
    bool enabled = true;
    std::string accessible_name = {};
    SliderOrientation orientation = SliderOrientation::horizontal;
    bool top_to_bottom = true;
    bool centered = false;
    std::optional<double> track_corner = std::nullopt;
    SliderValueLabel value_label = {};
};

class Slider {
public:
    explicit Slider(SliderOptions options = {});
    Slider(SliderOptions options, const text::FontStack& fonts);

    double width() const;
    double height() const;
    void set_width(double width);
    void set_length(double length) { length_ = length; }
    double length() const { return length_; }
    void set_origin(render::Point origin) { origin_ = origin; }
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;
    bool vertical() const { return options_.orientation == SliderOrientation::vertical; }

    double value() const { return options_.value; }
    void set_value(double value);
    double fraction() const;
    bool dragging() const { return dragging_; }
    bool holds_pointer() const { return dragging_; }
    double handle_width() const;
    double handle_center_x() const;
    render::Point handle_center() const;
    render::Rect handle_rect() const;
    std::vector<double> tick_centers() const;
    std::vector<TrackSegment> segments() const;
    bool value_indicator_shown() const { return indicator_.shown(); }
    double value_indicator_scale() const { return indicator_.scale(); }
    std::string value_text() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const;

    bool handle(const input::PointerEvent& event);
    void cancel_interaction();
    bool handle(const input::KeyEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    SliderAxis axis() const;
    double corner() const;
    double value_along() const;
    double value_at(double x, double y) const;
    void update_handle();

    SliderOptions options_;
    std::optional<text::FontStack> fonts_;
    double length_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool dragging_ = false;
    bool narrowed_ = false;
    FocusIndicator focus_;
    Enablement enabled_;
    ValueIndicator indicator_;
};

}
