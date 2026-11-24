#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/button_group.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/tokens/fab_menu.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/segmented_button.hpp"
#include "lamina/tokens/split_button.hpp"

namespace lamina {

struct Style {
    const color::ColorSpec* color = &color::spec_2025();

    tokens::ButtonSet button;
    tokens::ButtonGroupTokens button_group = tokens::button_group_tokens;
    tokens::IconButtonSet icon_button;
    tokens::FabSet fab;
    tokens::ToggleFabTokens toggle_fab = tokens::toggle_fab_tokens;
    tokens::FabMenuTokens fab_menu = tokens::fab_menu_tokens;
    tokens::SplitButtonSet split_button;
    tokens::SegmentedButtonTokens segmented_button = tokens::segmented_button_tokens;
    tokens::SegmentedButtonMotion segmented_button_motion = tokens::segmented_button_motion;
};

const Style& m3_expressive_style();

const Style& current_style();
void set_current_style(const Style& style);

class StyleScope {
public:
    explicit StyleScope(const Style& style);
    ~StyleScope();

    StyleScope(const StyleScope&) = delete;
    StyleScope& operator=(const StyleScope&) = delete;

private:
    const Style* previous_;
};

}
