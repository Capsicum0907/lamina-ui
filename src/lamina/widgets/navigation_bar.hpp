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
#include "lamina/tokens/navigation_bar.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/navigation_item.hpp"

namespace lamina::widgets {

struct NavigationBarOptions {
    std::vector<NavigationItem> items;
    std::size_t selected = 0;
    std::string accessible_name = {};
};

class NavigationBar {
public:
    NavigationBar(NavigationBarOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    std::size_t count() const { return items_.size(); }
    std::size_t selected() const { return options_.selected; }
    void select(std::size_t index);
    render::Rect item_rect(std::size_t index) const;
    render::Rect indicator_rect(std::size_t index) const;
    double indicator_progress(std::size_t index) const { return items_[index].indicator.value(); }

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

private:
    struct Item {
        std::optional<text::TextLine> label;
        PressState press;
        motion::Animated state_layer;
        motion::Animated indicator;
        motion::Animated color;
        Enablement enabled;
        bool allowed;
    };

    double content_top(std::size_t index) const;
    render::Rect full_indicator_rect(std::size_t index) const;

    NavigationBarOptions options_;
    text::FontStack fonts_;
    NavigationBadges badges_;
    std::vector<Item> items_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    FocusIndicator focus_;
    std::size_t focus_index_ = 0;

    void retarget_state_layers();
    void refresh_items();
    bool row_enabled_ = true;
};

}
