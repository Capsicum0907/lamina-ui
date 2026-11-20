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
#include "lamina/tokens/tabs.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

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
};

class TabRow {
public:
    TabRow(TabRowOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    void set_width(double width);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    std::size_t count() const { return tabs_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);
    render::Rect tab_rect(std::size_t index) const;
    render::Rect indicator_rect() const;
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
    void cancel_interaction();
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
    double content_width(const Tab& tab) const;
    double indicator_target_width() const;
    void retarget_indicator();
    void retarget_state_layers();
    void paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const;

    TabRowOptions options_;
    text::FontStack fonts_;
    std::vector<Tab> tabs_;
    double width_ = 0.0;
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool placed_ = false;
    motion::Animated indicator_offset_;
    motion::Animated indicator_width_;
    FocusIndicator focus_;
    bool row_enabled_ = true;

    void refresh_tabs();
};

}
