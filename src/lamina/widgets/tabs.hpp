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
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

enum class TabIconPosition { top, leading };

struct TabItem {
    std::string label;
    std::optional<icons::Icon> icon = std::nullopt;
};

struct TabRowOptions {
    std::vector<TabItem> tabs;
    tokens::TabStyle style = tokens::TabStyle::primary;
    std::size_t selected = 0;
    TabIconPosition icon_position = TabIconPosition::top;
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

    bool handle(const input::PointerEvent& event);
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
    };

    double tab_width() const;
    double content_width(const Tab& tab) const;
    double indicator_target_width() const;
    void retarget_indicator();
    void paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const;

    TabRowOptions options_;
    const text::FontStack& fonts_;
    std::vector<Tab> tabs_;
    double width_ = 0.0;
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool placed_ = false;
    motion::Animated indicator_offset_;
    motion::Animated indicator_width_;
};

}
