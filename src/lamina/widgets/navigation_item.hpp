#pragma once

#include <cstddef>
#include <string>

#include "lamina/icons/icon.hpp"

namespace lamina::widgets {

struct NavigationItem {
    icons::Icon icon;
    std::string label;
    icons::IconStyle style = {};
    icons::IconStyle selected_style = {};
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

}
