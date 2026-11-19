#include <doctest/doctest.h>

#include <utility>
#include <vector>

#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/item_focus.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

constexpr input::KeyEvent left{Key::left, false};
constexpr input::KeyEvent right{Key::right, false};
constexpr input::KeyEvent home{Key::home, false};
constexpr input::KeyEvent end{Key::end, false};
constexpr input::KeyEvent ctrl_right{Key::right, false, true};

template <typename W>
int ring_pixels(W& w, int width, int height) {
    thorvg::Canvas canvas(width, height);
    canvas.clear(0xffffffff);
    settle(w);
    w.paint(canvas, scheme());
    return count_color(canvas, scheme()[tokens::focus_indicator.color]);
}

widgets::TabRow tab_row() {
    std::vector<widgets::TabItem> items;
    for (const char* label : {"One", "Two", "Three"}) {
        items.push_back({label});
    }
    widgets::TabRow row({items}, fonts());
    row.set_width(300.0);
    row.set_origin({0.0, 10.0});
    return row;
}

std::vector<widgets::NavigationItem> navigation_items() {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{icons::Symbol::home, "Home"}, std::pair{icons::Symbol::search, "Search"},
                                 std::pair{icons::Symbol::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    return items;
}

widgets::NavigationBar navigation_bar() {
    widgets::NavigationBar bar({navigation_items()}, fonts());
    bar.set_width(360.0);
    return bar;
}

widgets::NavigationRail navigation_rail() {
    widgets::NavigationRail rail({navigation_items()}, fonts());
    rail.measure({0.0, 400.0, 0.0, 600.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    return rail;
}

template <typename W>
void check_item_focus(W w, input::KeyEvent forth, input::KeyEvent back, int width, int height) {
    CHECK_FALSE(w.handle(forth));
    CHECK_FALSE(w.handle(input::KeyEvent{Key::space, false}));
    const int unfocused = ring_pixels(w, width, height);
    w.set_focused(true);
    CHECK(w.focused_item() == 0);
    CHECK(ring_pixels(w, width, height) > unfocused);
    CHECK_FALSE(w.handle(forth));
    CHECK(w.focused_item() == 1);
    CHECK(w.selected() == 0);
    CHECK(w.handle(input::KeyEvent{Key::enter, false}));
    CHECK(w.selected() == 1);
    CHECK_FALSE(w.handle(input::KeyEvent{Key::space, false}));
    w.handle(back);
    w.handle(back);
    CHECK(w.focused_item() == 2);
    w.handle(home);
    CHECK(w.focused_item() == 0);
    w.handle(end);
    CHECK(w.focused_item() == 2);
    CHECK(w.handle(input::KeyEvent{Key::space, false}));
    CHECK(w.selected() == 2);
    CHECK_FALSE(w.handle(input::KeyEvent{Key::space, false, false, true}));
    w.set_focused(false);
    w.set_focused(true);
    CHECK(w.focused_item() == 2);
    w.handle({PointerAction::down, 1.0, 1.0});
    CHECK_FALSE(w.focused());
}

}

static_assert(widgets::Focusable<widgets::TabRow>);
static_assert(widgets::Focusable<widgets::NavigationBar>);
static_assert(widgets::Focusable<widgets::NavigationRail>);
static_assert(widgets::HandlesKeys<widgets::TabRow>);

TEST_CASE("item keys move along the axis, wrap at the ends and jump with Home and End") {
    using widgets::ItemAxis;
    using widgets::item_key_target;
    CHECK(item_key_target(right, 0, 3, ItemAxis::horizontal) == 1u);
    CHECK(item_key_target(right, 2, 3, ItemAxis::horizontal) == 0u);
    CHECK(item_key_target(left, 0, 3, ItemAxis::horizontal) == 2u);
    CHECK(item_key_target(home, 2, 3, ItemAxis::horizontal) == 0u);
    CHECK(item_key_target(end, 0, 3, ItemAxis::horizontal) == 2u);
    CHECK_FALSE(item_key_target(input::KeyEvent{Key::down, false}, 0, 3, ItemAxis::horizontal));
    CHECK(item_key_target(input::KeyEvent{Key::down, false}, 0, 3, ItemAxis::vertical) == 1u);
    CHECK(item_key_target(input::KeyEvent{Key::up, false}, 0, 3, ItemAxis::vertical) == 2u);
    CHECK_FALSE(item_key_target(ctrl_right, 0, 3, ItemAxis::horizontal));
    CHECK_FALSE(item_key_target(right, 0, 0, ItemAxis::horizontal));
}

TEST_CASE("a tab row takes the arrow keys only while it has the focus") {
    auto row = tab_row();
    CHECK_FALSE(row.handle(right));
    CHECK(row.selected() == 0);
    row.set_focused(true);
    CHECK(row.handle(right));
    CHECK(row.selected() == 1);
    CHECK(row.handle(end));
    CHECK(row.selected() == 2);
    CHECK(row.handle(right));
    CHECK(row.selected() == 0);
    CHECK(row.handle(left));
    CHECK(row.selected() == 2);
    CHECK(row.handle(home));
    CHECK(row.selected() == 0);
    CHECK_FALSE(row.handle(home));
    CHECK_FALSE(row.handle(ctrl_right));
    CHECK_FALSE(row.handle(input::KeyEvent{Key::space, false}));
    CHECK(row.selected() == 0);
}

TEST_CASE("a focused tab row draws the ring inside the selected tab, and a press takes the focus away") {
    auto row = tab_row();
    CHECK(ring_pixels(row, 300, 70) == 0);
    row.set_focused(true);
    CHECK(ring_pixels(row, 300, 70) > 0);
    row.handle({PointerAction::down, 250.0, 30.0});
    row.handle({PointerAction::up, 250.0, 30.0});
    CHECK_FALSE(row.focused());
    CHECK(row.selected() == 2);
    row.handle({PointerAction::leave, 0.0, 0.0});
    CHECK(ring_pixels(row, 300, 70) == 0);
}

TEST_CASE("a navigation bar moves its focus with Left and Right and selects on Space or Enter") {
    check_item_focus(navigation_bar(), right, left, 360, 64);
}

TEST_CASE("a navigation rail moves its focus with Up and Down and selects on Space or Enter") {
    check_item_focus(navigation_rail(), input::KeyEvent{Key::down, false}, input::KeyEvent{Key::up, false}, 120, 600);
}

namespace {

constexpr layout::Size menu_window{400.0, 400.0};
constexpr input::KeyEvent down{Key::down, false};
constexpr input::KeyEvent up{Key::up, false};

widgets::Menu three_item_menu() {
    return widgets::Menu({{{"Edit"}, {"Share"}, {"Copy"}}}, fonts());
}

}

static_assert(widgets::HandlesKeys<widgets::Menu>);

TEST_CASE("a menu opened by the pointer focuses no item until an arrow key, then moves and wraps") {
    auto m = three_item_menu();
    m.open({20.0, 20.0, 40.0, 40.0}, menu_window);
    CHECK_FALSE(m.focused_item());
    CHECK_FALSE(m.handle(input::KeyEvent{Key::enter, false}).chosen);
    CHECK(m.is_open());
    CHECK(m.handle(down).consumed);
    CHECK(m.focused_item() == 0u);
    CHECK(m.focus_shown());
    m.handle(up);
    CHECK(m.focused_item() == 2u);
    m.handle(down);
    CHECK(m.focused_item() == 0u);
    m.handle(end);
    CHECK(m.focused_item() == 2u);
    m.handle(home);
    CHECK(m.focused_item() == 0u);

    auto from_end = three_item_menu();
    from_end.open({20.0, 20.0, 40.0, 40.0}, menu_window);
    from_end.handle(up);
    CHECK(from_end.focused_item() == 2u);
}

TEST_CASE("a menu opened from the keyboard focuses its first item, and Enter or Space chooses it and closes") {
    auto m = three_item_menu();
    m.open({20.0, 20.0, 40.0, 40.0}, menu_window, input::InputSource::keyboard);
    CHECK(m.focused_item() == 0u);
    m.handle(down);
    CHECK_FALSE(m.handle(input::KeyEvent{Key::space, false, false, true}).chosen);
    const widgets::MenuResponse r = m.handle(input::KeyEvent{Key::space, false});
    CHECK(r.consumed);
    CHECK(r.chosen == 1u);
    CHECK_FALSE(m.is_open());
    CHECK_FALSE(m.focused_item());

    m.open({20.0, 20.0, 40.0, 40.0}, menu_window, input::InputSource::keyboard);
    CHECK(m.handle(input::KeyEvent{Key::enter, false}).chosen == 0u);
}

TEST_CASE("Escape and Tab close a menu without choosing") {
    for (const Key key : {Key::escape, Key::tab}) {
        auto m = three_item_menu();
        m.open({20.0, 20.0, 40.0, 40.0}, menu_window, input::InputSource::keyboard);
        const widgets::MenuResponse r = m.handle(input::KeyEvent{key, false});
        CHECK(r.consumed);
        CHECK_FALSE(r.chosen);
        CHECK_FALSE(m.is_open());
    }
}

TEST_CASE("the focused menu item shows the ring until a press inside the menu takes the keyboard focus away") {
    auto m = three_item_menu();
    m.open({20.0, 20.0, 40.0, 40.0}, menu_window);
    const int unfocused = ring_pixels(m, 400, 400);
    m.handle(down);
    CHECK(ring_pixels(m, 400, 400) > unfocused);
    const render::Rect item = m.item_rect(2);
    m.handle({PointerAction::down, item.x + 4.0, item.y + 4.0});
    CHECK_FALSE(m.focused_item());
    CHECK(ring_pixels(m, 400, 400) == unfocused);
}
