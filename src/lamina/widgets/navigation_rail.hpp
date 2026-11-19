#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/navigation_item.hpp"

namespace lamina::widgets {

struct NavigationRailOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
    bool expanded = false;
    layout::NodePtr header = nullptr;
};

class NavigationRail {
public:
    NavigationRail(NavigationRailOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    double measure_width(double max_width);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const;

    bool expanded() const { return options_.expanded; }
    void set_expanded(bool expanded);
    std::size_t count() const { return items_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);

    render::Rect item_rect(std::size_t index) const;
    render::Rect indicator_rect(std::size_t index) const;
    render::Rect state_layer_rect(std::size_t index) const;
    render::Rect icon_rect(std::size_t index) const;
    render::Rect label_rect(std::size_t index) const;
    double label_opacity(std::size_t index) const;
    double icon_position(std::size_t index) const { return items_[index].position.value(); }

    bool handle(const input::PointerEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        text::TextLine top_label;
        text::TextLine start_label;
        PressState press;
        motion::Animated state_layer;
        motion::Animated indicator;
        motion::Animated position;
        motion::Animated color;
        motion::Animated label_mode;
    };

    struct Geometry {
        double width;
        double height;
        double content_x;
        double content_y;
        double content_width;
        double content_height;
        double ripple_width;
        double ripple_height;
        double indicator_width;
        render::Point icon;
        render::Point label;
        const text::TextLine* line;
        double label_opacity;
    };

    Geometry geometry(std::size_t index, double item_max_width) const;
    double item_top(std::size_t index) const;
    double header_offset() const;
    void update_width();

    NavigationRailOptions options_;
    text::FontStack fonts_;
    std::vector<Item> items_;
    motion::Animated min_width_;
    motion::Animated full_width_;
    motion::Animated item_space_;
    motion::Animated item_min_height_;
    double max_width_;
    double width_ = 0.0;
    double current_width_ = 0.0;
    double max_expanded_width_ = 0.0;
    double item_max_width_ = 0.0;
    layout::Size header_size_{0.0, 0.0};
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
};

}
