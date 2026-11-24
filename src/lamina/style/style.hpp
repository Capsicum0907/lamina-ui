#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/access_key.hpp"
#include "lamina/tokens/badge.hpp"
#include "lamina/tokens/bottom_sheet.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/button_group.hpp"
#include "lamina/tokens/card.hpp"
#include "lamina/tokens/checkbox.hpp"
#include "lamina/tokens/chip.hpp"
#include "lamina/tokens/date_picker.hpp"
#include "lamina/tokens/dialog.hpp"
#include "lamina/tokens/divider.hpp"
#include "lamina/tokens/drag_handle.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/tokens/fab_menu.hpp"
#include "lamina/tokens/floating_toolbar.hpp"
#include "lamina/tokens/grouped_menu.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/list.hpp"
#include "lamina/tokens/loading_indicator.hpp"
#include "lamina/tokens/menu.hpp"
#include "lamina/tokens/menu_bar.hpp"
#include "lamina/tokens/navigation_bar.hpp"
#include "lamina/tokens/navigation_drawer.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/tokens/navigation_suite.hpp"
#include "lamina/tokens/numeric_field.hpp"
#include "lamina/tokens/pager.hpp"
#include "lamina/tokens/pane_scaffold.hpp"
#include "lamina/tokens/picker_dialog.hpp"
#include "lamina/tokens/progress_indicator.hpp"
#include "lamina/tokens/pull_to_refresh.hpp"
#include "lamina/tokens/radio_button.hpp"
#include "lamina/tokens/scrim.hpp"
#include "lamina/tokens/scroll_area.hpp"
#include "lamina/tokens/search.hpp"
#include "lamina/tokens/segmented_button.hpp"
#include "lamina/tokens/side_navigation.hpp"
#include "lamina/tokens/side_sheet.hpp"
#include "lamina/tokens/slider.hpp"
#include "lamina/tokens/snackbar.hpp"
#include "lamina/tokens/split_button.hpp"
#include "lamina/tokens/split_pane.hpp"
#include "lamina/tokens/switch.hpp"
#include "lamina/tokens/table.hpp"
#include "lamina/tokens/tabs.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/tokens/time_picker.hpp"
#include "lamina/tokens/title_bar.hpp"
#include "lamina/tokens/tooltip.hpp"
#include "lamina/tokens/top_app_bar.hpp"
#include "lamina/tokens/wavy_progress.hpp"

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
    
    tokens::OutlinedTextFieldTokens text_field = tokens::outlined_text_field;
    tokens::NumericFieldTokens numeric_field = tokens::numeric_field_tokens;
    tokens::CheckboxTokens checkbox = tokens::checkbox_tokens;
    tokens::RadioButtonTokens radio_button = tokens::radio_button_tokens;
    tokens::SwitchTokens switch_ = tokens::switch_tokens;
    tokens::SliderTokens slider = tokens::slider_tokens;
    tokens::ChipSet chip;
    tokens::SearchTokens search = tokens::search_tokens;
    tokens::SearchMotion search_motion = tokens::search_motion;
    tokens::DatePickerTokens date_picker = tokens::date_picker_tokens;
    tokens::DatePickerColors date_picker_colors = tokens::date_picker_colors;
    tokens::DatePickerMotion date_picker_motion = tokens::date_picker_motion;
    tokens::TimePickerTokens time_picker = tokens::time_picker_tokens;
    tokens::TimePickerColors time_picker_colors = tokens::time_picker_colors;
    tokens::TimePickerMotion time_picker_motion = tokens::time_picker_motion;
    tokens::PickerDialogTokens picker_dialog = tokens::picker_dialog_tokens;
    
    tokens::BottomSheetTokens bottom_sheet = tokens::bottom_sheet_tokens;
    tokens::SheetMotion bottom_sheet_motion = tokens::bottom_sheet_motion;
    tokens::SideSheetTokens side_sheet = tokens::side_sheet_tokens;
    motion::TweenSpec side_sheet_motion = tokens::side_sheet_motion;
    tokens::DialogTokens dialog = tokens::dialog_tokens;
    tokens::ScrimTokens scrim = tokens::scrim_tokens;
    tokens::MenuTokens menu = tokens::menu_tokens;
    tokens::MenuBarTokens menu_bar = tokens::menu_bar_tokens;
    tokens::GroupedMenuTokens grouped_menu = tokens::grouped_menu_tokens;
    tokens::PlainTooltipTokens plain_tooltip = tokens::plain_tooltip_tokens;
    tokens::RichTooltipTokens rich_tooltip = tokens::rich_tooltip_tokens;
    tokens::TooltipTokens tooltip = tokens::tooltip_tokens;
    tokens::SnackbarTokens snackbar = tokens::snackbar_tokens;
    tokens::DragHandleTokens drag_handle = tokens::drag_handle_tokens;
    tokens::AccessKeyTokens access_key = tokens::access_key_tokens;
    
    tokens::ListTokens list = tokens::list_tokens;
    tokens::TableTokens table = tokens::table_tokens;
    tokens::DividerTokens divider = tokens::divider_tokens;
    tokens::CardSet card;
    tokens::BadgeTokens badge = tokens::badge_tokens;
    tokens::ScrollAreaTokens scroll_area = tokens::scroll_area_tokens;
    
    tokens::CircularProgressTokens circular_progress = tokens::circular_progress_tokens;
    tokens::CircularIndeterminateTokens circular_indeterminate = tokens::circular_indeterminate;
    tokens::LinearIndeterminateTokens linear_indeterminate = tokens::linear_indeterminate;
    tokens::WavyProgressTokens wavy_progress = tokens::wavy_progress_tokens;
    tokens::WavyProgressMotion wavy_progress_motion = tokens::wavy_progress_motion;
    tokens::LoadingIndicatorSet loading_indicator;
    tokens::PullToRefreshTokens pull_to_refresh = tokens::pull_to_refresh_tokens;
    tokens::PullArrowTokens pull_arrow = tokens::pull_arrow_tokens;
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
