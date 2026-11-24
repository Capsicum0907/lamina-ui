#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/button_group.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/tokens/fab_menu.hpp"
#include "lamina/tokens/floating_toolbar.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/navigation_bar.hpp"
#include "lamina/tokens/navigation_drawer.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/tokens/navigation_suite.hpp"
#include "lamina/tokens/pager.hpp"
#include "lamina/tokens/pane_scaffold.hpp"
#include "lamina/tokens/segmented_button.hpp"
#include "lamina/tokens/side_navigation.hpp"
#include "lamina/tokens/split_button.hpp"
#include "lamina/tokens/split_pane.hpp"
#include "lamina/tokens/tabs.hpp"
#include "lamina/tokens/title_bar.hpp"
#include "lamina/tokens/top_app_bar.hpp"

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

    tokens::NavigationBarTokens navigation_bar = tokens::navigation_bar_tokens;
    tokens::NavigationRailTokens navigation_rail = tokens::navigation_rail_tokens;
    tokens::ModalNavigationRailTokens modal_navigation_rail = tokens::modal_navigation_rail_tokens;
    tokens::NavigationDrawerTokens navigation_drawer = tokens::navigation_drawer_tokens;
    tokens::DrawerMotion navigation_drawer_motion = tokens::navigation_drawer_motion;
    tokens::NavigationSuiteTokens navigation_suite = tokens::navigation_suite_tokens;
    tokens::SideNavigationTokens side_navigation = tokens::side_navigation_tokens;
    tokens::TabSet tabs;
    tokens::TopAppBarSet top_app_bar;
    tokens::TitleBarTokens title_bar = tokens::title_bar_tokens;
    tokens::PaneScaffoldTokens pane_scaffold = tokens::pane_scaffold_tokens;
    tokens::SplitPaneTokens split_pane = tokens::split_pane_tokens;
    tokens::PagerTokens pager = tokens::pager;
    tokens::FloatingToolbarTokens floating_toolbar = tokens::floating_toolbar_tokens;
    tokens::FlexibleBottomAppBarTokens flexible_bottom_app_bar = tokens::flexible_bottom_app_bar_tokens;
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
