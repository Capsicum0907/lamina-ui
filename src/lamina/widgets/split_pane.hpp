#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/split_pane.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/drag_handle.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

struct SplitPaneOptions {
    layout::Axis axis = layout::Axis::horizontal;
    layout::NodePtr first = nullptr;
    layout::NodePtr second = nullptr;
    double first_min = 0.0;
    double second_min = 0.0;
    std::optional<double> first_size = std::nullopt;
    double first_fraction = 0.5;
    std::string accessible_name = {};
    bool enabled = true;
    const PatternStrings* strings = &english_us_patterns();
};

class SplitPane {
public:
    explicit SplitPane(SplitPaneOptions options);

    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return rect_; }
    render::Rect first_rect() const { return first_rect_; }
    render::Rect second_rect() const { return second_rect_; }
    render::Rect divider_rect() const;
    render::Rect handle_rect() const;

    layout::Axis axis() const { return options_.axis; }
    double first_size() const { return first_; }
    double min_first() const;
    double max_first() const;
    void set_first_size(double size);
    bool hovered() const { return hovered_; }
    bool dragging() const { return dragging_; }
    std::optional<input::Cursor> cursor() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
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
    double main(const render::Rect& r) const { return options_.axis == layout::Axis::horizontal ? r.width : r.height; }
    double along(const input::PointerEvent& e) const { return options_.axis == layout::Axis::horizontal ? e.x_dp : e.y_dp; }
    double origin() const { return options_.axis == layout::Axis::horizontal ? rect_.x : rect_.y; }
    double available() const;
    double clamp_first(double size) const;
    void place();
    void retarget();

    SplitPaneOptions options_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    render::Rect first_rect_{0.0, 0.0, 0.0, 0.0};
    render::Rect second_rect_{0.0, 0.0, 0.0, 0.0};
    double first_ = 0.0;
    bool sized_ = false;
    bool hovered_ = false;
    bool dragging_ = false;
    double grab_ = 0.0;
    std::optional<double> restore_;
    FocusIndicator focus_;
    Enablement enabled_;
    DragHandlePainter handle_painter_;
    motion::Animated handle_shown_;
};

}
