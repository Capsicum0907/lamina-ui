#pragma once

#include <string>

#include "lamina/icons/icon.hpp"

namespace lamina::widgets {

struct NavigationItem {
    icons::Icon icon;
    std::string label;
    icons::IconStyle style = {};
    icons::IconStyle selected_style = {};
};

}
