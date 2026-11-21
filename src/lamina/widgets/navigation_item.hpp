#pragma once

#include <cstddef>
#include <string>

#include <optional>
#include <vector>

#include "lamina/icons/icon.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/widgets/badge.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

struct NavigationItem {
    icons::Icon icon;
    std::string label;
    icons::IconStyle style = {};
    icons::IconStyle selected_style = {};
    bool enabled = true;
    std::string accessible_name = {};
    std::optional<BadgeSpec> badge = std::nullopt;
};

class NavigationBadges {
public:
    NavigationBadges(const std::vector<NavigationItem>& items, const text::FontStack& fonts) : fonts_(fonts) {
        for (const NavigationItem& item : items) {
            badges_.push_back(item.badge ? std::optional<Badge>(Badge(*item.badge, fonts_)) : std::nullopt);
        }
    }

    void set(std::vector<NavigationItem>& items, std::size_t index, std::optional<BadgeSpec> spec) {
        if (index >= badges_.size()) {
            return;
        }
        items[index].badge = spec;
        badges_[index] = spec ? std::optional<Badge>(Badge(*spec, fonts_)) : std::nullopt;
    }
    const std::optional<Badge>& at(std::size_t index) const { return badges_[index]; }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, std::size_t index, const render::Rect& icon,
               const render::Rect& item) const {
        paint_badge(canvas, scheme, badges_[index], icon, item);
    }

private:
    text::FontStack fonts_;
    std::vector<std::optional<Badge>> badges_;
};

inline std::string badge_description(const std::optional<BadgeSpec>& badge) {
    if (!badge) {
        return {};
    }
    return badge->accessible_description.empty() ? badge->text : badge->accessible_description;
}

template <typename Items>
void select_navigation_item(Items& items, std::size_t& selected, std::size_t index) {
    if (index >= items.size()) {
        return;
    }
    selected = index;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const double on = i == index ? 1.0 : 0.0;
        items[i].indicator.animate_to(on);
        items[i].color.animate_to(on);
    }
}

inline void name_navigation_items(std::vector<NavigationItem>& items) {
    for (NavigationItem& item : items) {
        if (item.label.empty()) {
            item.accessible_name = required_name(std::move(item.accessible_name), "A navigation item without a label");
        }
    }
}

template <typename Items, typename Rect>
AccessibleNode navigation_accessible(const std::vector<NavigationItem>& specs, const Items& items, std::string name,
                                     const render::Rect& bounds, std::size_t selected, bool focused, std::size_t focus_index,
                                     Rect item_rect) {
    AccessibleNode node{AccessibleRole::navigation, std::move(name), bounds,
                        {.enabled = any_item_enabled(items), .focusable = any_item_enabled(items), .focused = focused}};
    for (std::size_t i = 0; i < specs.size(); ++i) {
        node.children.push_back({AccessibleRole::navigation_item, specs[i].accessible_name.empty() ? specs[i].label : specs[i].accessible_name,
                                 item_rect(i),
                                 {.enabled = items[i].enabled.enabled(), .focused = focused && i == focus_index, .selected = i == selected},
                                 {},
                                 badge_description(specs[i].badge)});
    }
    return node;
}

template <typename Items>
std::size_t navigation_focus_start(const Items& items, std::size_t selected) {
    if (selected < items.size() && items[selected].enabled.enabled()) {
        return selected;
    }
    return first_enabled_item(items.size(), [&](std::size_t i) { return items[i].enabled.enabled(); }).value_or(selected);
}

}
