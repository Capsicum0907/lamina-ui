#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/pane_scaffold.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/content_slot.hpp"
#include "lamina/widgets/drag_handle.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/pane_navigator.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

struct PaneExpansionAnchor {
    enum class Kind { proportion, offset_from_start, offset_from_end };
    Kind kind = Kind::proportion;
    double value = 0.5;

    static constexpr PaneExpansionAnchor proportion(double p) { return {Kind::proportion, p}; }
    static constexpr PaneExpansionAnchor from_start(double dp) { return {Kind::offset_from_start, dp}; }
    static constexpr PaneExpansionAnchor from_end(double dp) { return {Kind::offset_from_end, dp}; }

    double position_in(double total) const;
    std::string description(const PatternStrings& strings) const;
    constexpr bool operator==(const PaneExpansionAnchor&) const = default;
};

class PaneExpansionState {
public:
    explicit PaneExpansionState(std::vector<PaneExpansionAnchor> anchors = {}, std::optional<std::size_t> initial_anchor = std::nullopt);

    bool unspecified() const { return !first_width_ && !first_proportion_ && !offset_; }
    void set_first_pane_width(double width);
    void set_first_pane_proportion(double proportion);
    void clear();
    void animate_to(const PaneExpansionAnchor& anchor, double velocity = 0.0);
    void snap_to(const PaneExpansionAnchor& anchor);
    void nudge(double delta);

    const std::vector<PaneExpansionAnchor>& anchors() const { return anchors_; }
    std::optional<PaneExpansionAnchor> current_anchor() const;
    std::optional<PaneExpansionAnchor> next_anchor() const;
    std::optional<double> first_pane_width() const { return first_width_; }
    std::optional<double> first_pane_proportion() const { return first_proportion_; }
    std::optional<double> offset() const { return offset_; }
    double measured_offset() const { return measured_offset_; }
    double max_width() const { return width_; }
    bool dragging() const { return dragging_; }
    bool settling() const { return settling_; }

    void on_measured(double width);
    void on_offset_measured(double offset) { measured_offset_ = offset; }
    void press(double x, double time_seconds);
    void drag(double x, double time_seconds);
    void release(double time_seconds);
    void cancel();
    void step(double dt_seconds);
    bool animating() const { return settling_; }

private:
    std::size_t closest(double position, double velocity) const;
    double current() const { return offset_.value_or(measured_offset_); }

    std::vector<PaneExpansionAnchor> anchors_;
    std::optional<std::size_t> current_;
    std::optional<double> first_width_;
    std::optional<double> first_proportion_;
    std::optional<double> offset_;
    double measured_offset_ = 0.0;
    double width_ = 0.0;
    bool dragging_ = false;
    bool settling_ = false;
    double grab_ = 0.0;
    double velocity_ = 0.0;
    double target_ = 0.0;
    motion::VelocityTracker tracker_;
};

struct ThreePaneScaffoldOptions {
    PaneOrder order = list_detail_pane_order;
    std::array<std::optional<ContentSlot>, pane_count> panes = {};
    std::array<std::string, pane_count> names = {};
    std::array<std::optional<double>, pane_count> preferred_widths = {};
    bool expansion_handle = false;
    std::vector<PaneExpansionAnchor> anchors = {};
    std::optional<std::size_t> initial_anchor = std::nullopt;
    bool animate_directive_changes = false;
    const PatternStrings* strings = &english_us_patterns();
};

struct PaneScaffoldResponse {
    bool navigated_back = false;
    bool consumed = false;
};

class ThreePaneScaffold {
public:
    ThreePaneScaffold(ThreePaneScaffoldOptions options, ThreePaneScaffoldNavigator& navigator);

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return rect_; }

    ThreePaneScaffoldNavigator& navigator() { return navigator_; }
    const ThreePaneScaffoldNavigator& navigator() const { return navigator_; }
    PaneExpansionState& expansion() { return expansion_; }
    const PaneExpansionState& expansion() const { return expansion_; }
    bool pane_shown(PaneRole role) const;
    bool pane_visible(PaneRole role) const;
    render::Rect pane_rect(PaneRole role) const;
    render::Rect target_pane_rect(PaneRole role) const;
    std::optional<render::Rect> handle_rect() const;
    bool back();
    std::optional<input::Cursor> cursor() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool can_focus() const;
    render::Rect bounds_for_focus() const;
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);

    PaneScaffoldResponse handle(const input::PointerEvent& event);
    bool holds_pointer() const { return expansion_.dragging(); }
    PaneScaffoldResponse handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::vector<AccessibleNode> accessible_nodes() const;

private:
    struct Pane {
        motion::Animated x;
        motion::Animated y;
        motion::Animated width;
        motion::Animated height;
        render::Rect target{0.0, 0.0, 0.0, 0.0};
        bool shown = false;
        bool visible = false;
        bool placed = false;
        double delay = 0.0;
    };

    void compute_targets(std::array<render::Rect, pane_count>& out, std::array<bool, pane_count>& shown);
    void apply(const PaneScaffoldValue& value, bool animate);
    void refresh_targets();
    void focus_destination(bool had_focus);
    bool any_pane_focused() const;
    void sync_handle();
    WidgetSet* widgets(PaneRole role) const;
    FocusManager* pane_focus(PaneRole role) const;
    std::size_t index(PaneRole role) const { return static_cast<std::size_t>(role); }

    ThreePaneScaffoldOptions options_;
    ThreePaneScaffoldNavigator& navigator_;
    PaneExpansionState expansion_;
    PaneScaffoldValue applied_;
    PaneScaffoldDirective applied_directive_;
    bool started_ = false;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    std::array<Pane, pane_count> panes_;
    std::optional<render::Rect> handle_;
    bool handle_hovered_ = false;
    bool handle_pressed_ = false;
    DragHandlePainter handle_painter_;
    FocusIndicator focus_;
    Enablement enabled_;
    double clock_ = 0.0;
    StyleStamp style_stamp_;
};

}
