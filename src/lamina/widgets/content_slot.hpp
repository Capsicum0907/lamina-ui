#pragma once

#include "lamina/layout/layout.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/widget_set.hpp"

namespace lamina::widgets {

struct ContentSlot {
    layout::NodePtr node;
    WidgetSet* widgets = nullptr;
    FocusManager* focus = nullptr;
};

}
