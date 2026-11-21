#include "lamina/widgets/pattern_strings.hpp"

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
    };
    return strings;
}

}
