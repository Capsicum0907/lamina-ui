#pragma once

#include <string>
#include <string_view>

namespace lamina::widgets {

struct PatternStrings {
    std::string bottom_sheet_pane_title;
    std::string drag_handle;
    std::string collapse_bottom_sheet;
    std::string dismiss_bottom_sheet;
    std::string expand_bottom_sheet;
    std::string close_sheet;
    std::string side_sheet_pane_title;
    std::string navigation_menu;
    std::string close_drawer;
    std::string close_rail;
    std::string navigation_rail_pane_title;
    std::string search;
    std::string suggestions_below;
    std::string more_options;
    std::string expand_toolbar;
    std::string collapse_toolbar;
    std::string primary_pane;
    std::string secondary_pane;
    std::string tertiary_pane;
    std::string pane_expansion_drag_handle;
    std::string pane_split_state;
    std::string pane_split_action;
    std::string proportion_anchor;
    std::string start_offset_anchor;
    std::string end_offset_anchor;
    std::string pane_divider;
};

std::string format_pattern(std::string_view pattern, std::string_view value);

const PatternStrings& english_us_patterns();
const PatternStrings& english_gb_patterns();
const PatternStrings& japanese_patterns();

}
