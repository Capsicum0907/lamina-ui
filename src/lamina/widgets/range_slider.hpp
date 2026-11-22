#pragma once

#include <array>
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
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/slider_track.hpp"

namespace lamina::widgets {

enum class RangeThumb { start, end };

struct RangeSliderNames {
    std::string start = "Range start";
    std::string end = "Range end";
};

struct RangeSliderOptions {
    double start = 0.0;
    double end = 1.0;
    double min = 0.0;
    double max = 1.0;
    int steps = 0;
    bool enabled = true;
    std::string accessible_name = {};
    RangeSliderNames names = {};
    SliderValueLabel value_label = {};
};

class RangeSlider;

class RangeSliderThumb {
public:
    RangeSliderThumb(RangeSlider& slider, RangeThumb which) : slider_(slider), which_(which) {}

    bool focused() const;
    void set_focused(bool focused);
    bool can_focus() const;
    render::Rect bounds() const;
    RangeThumb which() const { return which_; }

private:
    RangeSlider& slider_;
    RangeThumb which_;
};

class RangeSlider {
public:
    explicit RangeSlider(RangeSliderOptions options = {});
    RangeSlider(RangeSliderOptions options, const text::FontStack& fonts);
    RangeSlider(const RangeSlider&) = delete;
    RangeSlider& operator=(const RangeSlider&) = delete;

    double width() const { return width_; }
    double height() const;
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin) { origin_ = origin; }
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;

    double start() const { return values_[0]; }
    double end() const { return values_[1]; }
    void set_range(double start, double end);
    double value(RangeThumb thumb) const { return values_[index(thumb)]; }
    std::optional<RangeThumb> dragging() const { return dragging_; }
    bool holds_pointer() const { return dragging_.has_value(); }
    RangeSliderThumb& thumb(RangeThumb which) { return thumbs_[index(which)]; }
    const RangeSliderThumb& thumb(RangeThumb which) const { return thumbs_[index(which)]; }
    render::Rect handle_rect(RangeThumb thumb) const;
    double handle_width(RangeThumb thumb) const;
    std::vector<TrackSegment> segments() const;
    std::vector<double> tick_centers() const;
    std::string value_text(RangeThumb thumb) const;
    bool value_indicator_shown(RangeThumb thumb) const { return indicators_[index(thumb)].shown(); }

    std::optional<RangeThumb> focused_thumb() const;
    bool focused(RangeThumb thumb) const { return focus_[index(thumb)].focused(); }
    void set_focused(RangeThumb thumb, bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    AccessibleNode accessible() const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    static std::size_t index(RangeThumb t) { return t == RangeThumb::start ? 0 : 1; }
    SliderAxis axis() const;
    double corner() const;
    double fraction(double v) const;
    double along(RangeThumb thumb) const;
    double value_at(double x) const;
    void move(RangeThumb thumb, double value);
    void update_handles();

    RangeSliderOptions options_;
    std::optional<text::FontStack> fonts_;
    std::array<double, 2> values_{0.0, 1.0};
    std::array<RangeSliderThumb, 2> thumbs_;
    std::array<FocusIndicator, 2> focus_;
    std::array<ValueIndicator, 2> indicators_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    std::optional<RangeThumb> dragging_;
    bool choosing_by_direction_ = false;
    double press_x_ = 0.0;
    Enablement enabled_;
};

}
