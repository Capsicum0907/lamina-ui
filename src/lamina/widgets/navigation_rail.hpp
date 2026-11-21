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
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/navigation_item.hpp"

namespace lamina::widgets {

struct NavigationRailOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
    bool expanded = false;
    layout::NodePtr header = nullptr;
    std::string accessible_name = {};
    tokens::Density density = {};
};

class NavigationRail {
public:
    NavigationRail(NavigationRailOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const;

    bool expanded() const { return options_.expanded; }
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);
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

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool item_enabled(std::size_t index) const { return items_[index].enabled.enabled(); }
    void set_item_enabled(std::size_t index, bool enabled);
    void set_item_badge(std::size_t index, std::optional<BadgeSpec> badge) { badges_.set(options_.items, index, std::move(badge)); }
    const std::optional<Badge>& item_badge(std::size_t index) const { return badges_.at(index); }
    bool enabled() const { return row_enabled_; }
    void set_enabled(bool enabled);
    bool can_focus() const { return row_enabled_ && any_item_enabled(items_); }
    AccessibleNode accessible() const {
        return navigation_accessible(options_.items, items_, options_.accessible_name, bounds(), options_.selected, focused(), focus_index_,
                                     [this](std::size_t i) { return item_rect(i); });
    }
    std::size_t focused_item() const { return focus_index_; }

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_items(render::Canvas& canvas, const color::ColorScheme& scheme) const;

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
        Enablement enabled;
        bool allowed;
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
        bool start_label;
        double label_opacity;
    };

    Geometry geometry(std::size_t index, double item_max_width) const;
    double item_min_height(bool expanded) const;
    const text::TextLine& label_line(std::size_t index) const;
    double header_offset() const;
    void update_width();
    void update_layout();

    NavigationRailOptions options_;
    text::FontStack fonts_;
    NavigationBadges badges_;
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
    std::vector<Geometry> layout_;
    std::vector<double> item_tops_;
    layout::Size header_size_{0.0, 0.0};
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;

    void retarget_state_layers();
    void refresh_items();
    bool row_enabled_ = true;
};

}
