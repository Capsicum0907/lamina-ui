#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/navigation_item.hpp"

namespace lamina::widgets {

struct SideNavigationOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
    bool expanded = false;
    bool drawer = false;
    std::string accessible_name = {};
    tokens::Density density = {};
};

class SideNavigation {
public:
    SideNavigation(SideNavigationOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    double width_at(double expansion, double drawer) const;
    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height_}; }

    bool expanded() const { return options_.expanded; }
    void set_expanded(bool expanded);
    bool drawer() const { return options_.drawer; }
    void set_drawer(bool drawer);
    double expansion() const;
    double drawer_amount() const;
    double label_opacity() const;
    void snap_to_targets();

    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);
    double row_height() const;

    std::size_t count() const { return items_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);

    render::Rect item_rect(std::size_t index) const;
    render::Rect shape_rect(std::size_t index) const;
    render::Rect indicator_rect(std::size_t index) const;
    render::Rect icon_rect(std::size_t index) const;
    render::Rect label_rect(std::size_t index) const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    std::size_t focused_item() const { return focus_index_; }
    bool item_enabled(std::size_t index) const { return items_[index].enabled.enabled(); }
    void set_item_enabled(std::size_t index, bool enabled);
    void set_item_badge(std::size_t index, std::optional<BadgeSpec> badge) { badges_.set(options_.items, index, std::move(badge)); }
    const std::optional<Badge>& item_badge(std::size_t index) const { return badges_.at(index); }
    bool enabled() const { return row_enabled_; }
    void set_enabled(bool enabled);
    bool can_focus() const { return row_enabled_ && any_item_enabled(items_); }
    AccessibleNode accessible() const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        text::TextLine label;
        PressState press;
        motion::Animated state_layer;
        motion::Animated indicator;
        motion::Animated color;
        Enablement enabled;
        bool allowed;
    };

    double hug_width(std::size_t index) const;
    double rail_width() const;
    double drawer_width() const;
    double row_top(std::size_t index) const;
    void update_width();
    void retarget_state_layers();
    void refresh_items();

    SideNavigationOptions options_;
    text::FontStack fonts_;
    NavigationBadges badges_;
    std::vector<Item> items_;
    motion::Animated expansion_;
    motion::Animated drawer_;
    double max_width_;
    double width_ = 0.0;
    double height_ = 0.0;
    render::Point origin_{0.0, 0.0};
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;
    bool row_enabled_ = true;
};

}
