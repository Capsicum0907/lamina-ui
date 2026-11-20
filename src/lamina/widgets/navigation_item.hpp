#pragma once

#include <cstddef>
#include <string>

#include <optional>
#include <vector>

#include "lamina/icons/icon.hpp"
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
};

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
                                 {.enabled = items[i].enabled.enabled(), .focused = focused && i == focus_index, .selected = i == selected}});
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
