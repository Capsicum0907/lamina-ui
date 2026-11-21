#include "lamina/widgets/pattern_strings.hpp"

#include <initializer_list>

namespace lamina::widgets {

const PatternStrings& english_us_patterns() {
    static const PatternStrings strings{
        .bottom_sheet_pane_title = "Bottom Sheet",
        .drag_handle = "Drag handle",
        .collapse_bottom_sheet = "Collapse bottom sheet",
        .dismiss_bottom_sheet = "Dismiss bottom sheet",
        .expand_bottom_sheet = "Expand bottom sheet",
        .close_sheet = "Close sheet",
        .side_sheet_pane_title = "Side Sheet",
        .navigation_menu = "Navigation menu",
        .close_drawer = "Close navigation menu",
        .close_rail = "Close rail",
        .navigation_rail_pane_title = "Navigation rail",
        .search = "Search",
        .suggestions_below = "Suggestions below",
        .more_options = "More options",
        .expand_toolbar = "Expand toolbar",
        .collapse_toolbar = "Collapse toolbar",
        .primary_pane = "Primary pane",
        .secondary_pane = "Secondary pane",
        .tertiary_pane = "Tertiary pane",
        .pane_expansion_drag_handle = "Pane expansion drag handle",
        .pane_split_state = "Current pane split, %s",
        .pane_split_action = "Change pane split to %s",
        .proportion_anchor = "%d percent",
        .start_offset_anchor = "%d DPs from start",
        .end_offset_anchor = "%d DPs from end",
        .pane_divider = "Pane divider",
    };
    return strings;
}

const PatternStrings& english_gb_patterns() {
    static const PatternStrings strings = [] {
        PatternStrings s = english_us_patterns();
        s.bottom_sheet_pane_title = "Bottom sheet";
        s.side_sheet_pane_title = "Side sheet";
        return s;
    }();
    return strings;
}

const PatternStrings& japanese_patterns() {
    static const PatternStrings strings{
        .bottom_sheet_pane_title = "ボトムシート",
        .drag_handle = "ドラッグ ハンドル",
        .collapse_bottom_sheet = "ボトムシートを折りたたみます",
        .dismiss_bottom_sheet = "ボトムシートを閉じます",
        .expand_bottom_sheet = "ボトムシートを開きます",
        .close_sheet = "シートを閉じる",
        .side_sheet_pane_title = "サイドシート",
        .navigation_menu = "ナビゲーションメニュー",
        .close_drawer = "ナビゲーションメニューを閉じる",
        .close_rail = "レールを閉じる",
        .navigation_rail_pane_title = "ナビゲーション レール",
        .search = "検索",
        .suggestions_below = "検索候補は次のとおりです",
        .more_options = "その他のオプション",
        .expand_toolbar = "ツールバーを開きます",
        .collapse_toolbar = "ツールバーを閉じます",
        .primary_pane = "プライマリ ペイン",
        .secondary_pane = "セカンダリ ペイン",
        .tertiary_pane = "ターシャリ ペイン",
        .pane_expansion_drag_handle = "ペインの展開のドラッグ ハンドル",
        .pane_split_state = "現在のペイン分割、%s",
        .pane_split_action = "ペインの分割を %s に変更",
        .proportion_anchor = "%d パーセント",
        .start_offset_anchor = "開始地点から %d DP",
        .end_offset_anchor = "終了地点から %d DP",
        .pane_divider = "ペインの仕切り",
    };
    return strings;
}

std::string format_pattern(std::string_view pattern, std::string_view value) {
    for (const std::string_view slot : {std::string_view("%s"), std::string_view("%d")}) {
        if (const std::size_t at = pattern.find(slot); at != std::string_view::npos) {
            std::string out(pattern.substr(0, at));
            out += value;
            out += pattern.substr(at + slot.size());
            return out;
        }
    }
    return std::string(pattern);
}

}
