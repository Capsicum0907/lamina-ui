#pragma once

#include <cstddef>
#include <string>

#include <optional>

#include "lamina/icons/icon.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

struct NavigationItem {
    icons::Icon icon;
    std::string label;
    icons::IconStyle style = {};
    icons::IconStyle selected_style = {};
    bool enabled = true;
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

template <typename Items>
std::size_t navigation_focus_start(const Items& items, std::size_t selected) {
    if (selected < items.size() && items[selected].enabled.enabled()) {
        return selected;
    }
    return first_enabled_item(items.size(), [&](std::size_t i) { return items[i].enabled.enabled(); }).value_or(selected);
}

}
