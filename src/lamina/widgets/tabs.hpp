#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/spline_decay.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/tabs.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/touch_scroll.hpp"

namespace lamina::widgets {

enum class TabIconPosition { top, leading };

struct TabItem {
    std::string label;
    std::optional<icons::Icon> icon = std::nullopt;
    bool enabled = true;
    std::string accessible_name = {};
};

struct TabRowOptions {
    std::vector<TabItem> tabs;
    tokens::TabStyle style = tokens::TabStyle::primary;
    std::size_t selected = 0;
    TabIconPosition icon_position = TabIconPosition::top;
    std::string accessible_name = {};
    tokens::Density density = {};
    bool scrollable = false;
    double edge_padding = current_style().tabs.common.scrollable_edge_padding;
};

class TabRow {
public:
    TabRow(TabRowOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);
    void set_width(double width);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    bool scrollable() const { return options_.scrollable; }
    double scroll_extent() const;
    double scroll_offset() const { return scroll_.value(); }
    double max_scroll() const;
    bool dragging() const { return touch_.owns(); }
    bool flinging() const { return fling_.active(); }
    bool holds_pointer() const { return dragging(); }

    std::size_t count() const { return tabs_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);
    render::Rect tab_rect(std::size_t index) const;
    render::Rect indicator_rect() const;
    void set_indicator_position(double position);
    double selection(std::size_t index) const { return tabs_[index].selected.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool tab_enabled(std::size_t index) const { return tabs_[index].enabled.enabled(); }
    void set_tab_enabled(std::size_t index, bool enabled);
    bool enabled() const { return row_enabled_; }
    void set_enabled(bool enabled);
    bool can_focus() const;
    AccessibleNode accessible() const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Tab {
        std::optional<text::TextLine> label;
        std::optional<icons::Icon> icon;
        PressState press;
        motion::Animated state_layer;
        motion::Animated selected;
        Enablement enabled;
        bool allowed;
    };

    double tab_width() const;
    double slot_start(std::size_t index) const;
    double slot_width(std::size_t index) const;
    double natural_width(const Tab& tab) const;
    double content_width(const Tab& tab, double slot) const;
    double centred_scroll(std::size_t index) const;
    render::Rect visible_rect(const render::Rect& r) const;
    void clamp_scroll();
    std::optional<bool> handle_touch(const input::PointerEvent& event);
    void follow_touch(double dx);
    double indicator_width_for(std::size_t index) const;
    double indicator_target_width() const;
    void retarget_indicator();
    void retarget_state_layers();
    void paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const;

    TabRowOptions options_;
    text::FontStack fonts_;
    std::vector<Tab> tabs_;
    double width_ = 0.0;
    double height_ = 0.0;
    bool stacked_ = false;
    void update_height();
    render::Point origin_{0.0, 0.0};
    bool placed_ = false;
    motion::Animated indicator_offset_;
    motion::Animated indicator_width_;
    motion::Animated scroll_;
    TouchScroll touch_;
    motion::Fling fling_;
    std::vector<double> natural_widths_;
    FocusIndicator focus_;
    bool row_enabled_ = true;
    StyleStamp style_stamp_;

    void refresh_tabs();
};

}
