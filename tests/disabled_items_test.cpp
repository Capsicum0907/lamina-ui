#include <doctest/doctest.h>

#include <utility>
#include <vector>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/tabs.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

constexpr input::KeyEvent right{Key::right, false};
constexpr input::KeyEvent down{Key::down, false};
constexpr input::KeyEvent up{Key::up, false};
constexpr input::KeyEvent home{Key::home, false};
constexpr input::KeyEvent end{Key::end, false};
constexpr input::KeyEvent enter{Key::enter, false};

widgets::TabRow tab_row(bool second_enabled) {
    std::vector<widgets::TabItem> items{{"One"}, {"Two", std::nullopt, second_enabled}, {"Three"}};
    widgets::TabRow row({items}, fonts());
    row.set_width(300.0);
    row.set_origin({0.0, 10.0});
    return row;
}

std::vector<widgets::NavigationItem> navigation_items(bool first_enabled) {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{res::icons::home, "Home"}, std::pair{res::icons::search, "Search"},
                                 std::pair{res::icons::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    items[0].enabled = first_enabled;
    return items;
}

}

TEST_CASE("the item keys skip disabled items") {
    auto enabled = [](std::size_t i) { return i != 1; };
    CHECK(widgets::item_key_target(right, 0, 3, widgets::ItemAxis::horizontal, enabled) == 2u);
    CHECK(widgets::item_key_target(input::KeyEvent{Key::left}, 2, 3, widgets::ItemAxis::horizontal, enabled) == 0u);
    auto last_off = [](std::size_t i) { return i != 2; };
    CHECK(widgets::item_key_target(end, 0, 3, widgets::ItemAxis::horizontal, last_off) == 1u);
    auto first_off = [](std::size_t i) { return i != 0; };
    CHECK(widgets::item_key_target(home, 2, 3, widgets::ItemAxis::horizontal, first_off) == 1u);
    CHECK(widgets::menu_key_target(down, std::nullopt, 3, first_off) == 1u);
    auto none = [](std::size_t) { return false; };
    CHECK_FALSE(widgets::item_key_target(right, 0, 3, widgets::ItemAxis::horizontal, none));
    CHECK_FALSE(widgets::first_enabled_item(3, none));
}

TEST_CASE("a disabled tab takes no click and the arrows pass over it") {
    auto row = tab_row(false);
    CHECK_FALSE(row.tab_enabled(1));
    CHECK_FALSE(click(row, 150.0, 30.0));
    CHECK(row.selected() == 0);
    row.set_focused(true);
    CHECK(row.handle(right));
    CHECK(row.selected() == 2);
    row.set_tab_enabled(1, true);
    CHECK(click(row, 150.0, 30.0));
    CHECK(row.selected() == 1);
}

TEST_CASE("a tab row with every tab disabled leaves the focus order") {
    auto row = tab_row(true);
    row.set_focused(true);
    row.set_enabled(false);
    CHECK_FALSE(row.can_focus());
    CHECK_FALSE(row.focused());
    widgets::FocusManager focus;
    focus.add(row);
    CHECK_FALSE(focus.move_focus(widgets::FocusDirection::next));
    row.set_enabled(true);
    CHECK(focus.move_focus(widgets::FocusDirection::next));
    CHECK(row.focused());
}

TEST_CASE("the navigation bar starts its focus on an enabled item and selects only enabled ones") {
    widgets::NavigationBar bar({navigation_items(false), 0}, fonts());
    bar.set_width(360.0);
    bar.set_focused(true);
    CHECK(bar.focused_item() == 1);
    bar.handle(right);
    CHECK(bar.focused_item() == 2);
    bar.handle(right);
    CHECK(bar.focused_item() == 1);
    CHECK_FALSE(click(bar, 60.0, 30.0));
    CHECK(bar.selected() == 0);
    CHECK_FALSE(bar.focused());
    bar.set_focused(true);
    CHECK(bar.handle(enter));
    CHECK(bar.selected() == 1);
}

TEST_CASE("disabling the focused rail item moves the focus to the next enabled one") {
    widgets::NavigationRail rail({navigation_items(true)}, fonts());
    rail.measure({0.0, 400.0, 0.0, 600.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    rail.set_focused(true);
    CHECK(rail.focused_item() == 0);
    rail.set_item_enabled(0, false);
    CHECK(rail.focused_item() == 1);
    rail.handle(up);
    CHECK(rail.focused_item() == 2);
    rail.handle(input::KeyEvent{Key::down});
    CHECK(rail.focused_item() == 1);
}

TEST_CASE("a menu skips its disabled items for the keys, the pointer and the access keys") {
    widgets::Menu menu({{{"Cut"}, {"Copy", std::nullopt, std::nullopt, false}, {"Paste"}}}, fonts());
    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0}, input::InputSource::keyboard);
    CHECK(menu.focused_item() == 0u);
    menu.handle(down);
    CHECK(menu.focused_item() == 2u);
    menu.handle(up);
    CHECK(menu.focused_item() == 0u);
    const auto copy_key = menu.handle(input::KeyEvent{Key::c});
    CHECK(copy_key.consumed);
    CHECK(copy_key.chosen == 0u);

    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0});
    const render::Rect copy = menu.item_rect(1);
    menu.handle({PointerAction::down, copy.x + 10.0, copy.y + 10.0});
    const auto released = menu.handle({PointerAction::up, copy.x + 10.0, copy.y + 10.0});
    CHECK_FALSE(released.chosen);
    CHECK(menu.is_open());
}

TEST_CASE("a menu opened from the keyboard focuses its first enabled item") {
    widgets::Menu menu({{{"Cut", std::nullopt, std::nullopt, false}, {"Copy"}}}, fonts());
    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0}, input::InputSource::keyboard);
    CHECK(menu.focused_item() == 1u);
}

TEST_CASE("a disabled grouped menu item takes no keys, clicks or access keys") {
    widgets::GroupedMenuItem off{"Share"};
    off.enabled = false;
    widgets::GroupedMenu menu({{widgets::MenuGroup{{off, widgets::GroupedMenuItem{"Open"}, widgets::GroupedMenuItem{"Settings"}}}}},
                              fonts());
    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0}, input::InputSource::keyboard);
    CHECK(menu.focused_item()->item == 1u);
    const auto s = menu.handle(input::KeyEvent{Key::s});
    CHECK(s.chosen);
    CHECK(s.chosen->item == 2u);
    menu.open({10.0, 10.0, 40.0, 40.0}, {600.0, 600.0});
    const render::Rect share = menu.item_rect(0, 0);
    menu.handle({PointerAction::down, share.x + 10.0, share.y + 10.0});
    CHECK_FALSE(menu.handle({PointerAction::up, share.x + 10.0, share.y + 10.0}).chosen);
    menu.set_item_enabled(0, 0, true);
    CHECK(menu.item_enabled(0, 0));
}

TEST_CASE("disabling and enabling a whole row keeps the items that were disabled on their own") {
    auto row = tab_row(false);
    row.set_enabled(false);
    CHECK_FALSE(row.enabled());
    CHECK_FALSE(row.tab_enabled(0));
    row.set_enabled(true);
    CHECK(row.tab_enabled(0));
    CHECK_FALSE(row.tab_enabled(1));

    widgets::NavigationBar bar({navigation_items(false)}, fonts());
    bar.set_enabled(false);
    CHECK_FALSE(bar.can_focus());
    bar.set_enabled(true);
    CHECK_FALSE(bar.item_enabled(0));
    CHECK(bar.item_enabled(1));
}
