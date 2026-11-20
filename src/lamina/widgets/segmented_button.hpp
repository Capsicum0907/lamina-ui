#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/segmented_button.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

enum class SegmentSelection { single, multiple };

struct Segment {
    std::string label;
    std::optional<icons::Icon> icon = std::nullopt;
    bool checked = false;
    bool enabled = true;
    std::string accessible_name = {};
};

struct SegmentedButtonRowOptions {
    std::vector<Segment> segments;
    SegmentSelection selection = SegmentSelection::single;
    std::string accessible_name = {};
    bool enabled = true;
};

struct SegmentedButtonResponse {
    std::optional<std::size_t> clicked;
    bool consumed = false;
};

class SegmentedButtonRow {
public:
    SegmentedButtonRow(SegmentedButtonRowOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);

    double width() const;
    double height() const;
    double segment_width() const { return segment_width_; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }
    render::Rect segment_bounds(std::size_t index) const;
    std::size_t size() const { return segments_.size(); }
    SegmentSelection selection() const { return options_.selection; }

    bool checked(std::size_t index) const { return segments_[index].spec.checked; }
    void set_checked(std::size_t index, bool checked);
    std::optional<std::size_t> selected() const;
    std::vector<std::size_t> checked_segments() const;
    double check_presence(std::size_t index) const { return segments_[index].check_scale.value(); }
    double label_offset(std::size_t index) const { return segments_[index].label_offset.value(); }
    const std::string& segment_name(std::size_t index) const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    std::optional<std::size_t> focused_segment() const;
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    bool segment_enabled(std::size_t index) const { return segments_[index].enabled.enabled(); }
    void set_segment_enabled(std::size_t index, bool enabled);
    bool can_focus() const;
    AccessibleNode accessible() const;

    SegmentedButtonResponse handle(const input::PointerEvent& event);
    SegmentedButtonResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        Segment spec;
        text::TextLine label;
        PressState press;
        motion::Animated state_layer;
        motion::Animated checked_amount;
        motion::Animated check_scale;
        motion::Animated check_alpha;
        motion::Animated icon_cross;
        motion::Animated label_offset;
        Enablement enabled;
        bool allowed;
    };

    void activate(std::size_t index, SegmentedButtonResponse& response);
    void apply_checked(std::size_t index, bool checked);
    void retarget();
    int z_of(std::size_t index) const;
    void paint_segment(render::Canvas& canvas, const color::ColorScheme& scheme, std::size_t index) const;

    SegmentedButtonRowOptions options_;
    text::FontStack fonts_;
    icons::Icon checkmark_;
    std::vector<Item> segments_;
    double segment_width_ = 0.0;
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;
    bool enabled_ = true;
};

}
