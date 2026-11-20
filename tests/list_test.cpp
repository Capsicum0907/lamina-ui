#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::ListItem;
using widgets::ListItemAction;
using widgets::ListItemSpec;
using widgets::ListLines;
using widgets::ListSlot;
using widgets::ListView;

namespace {

icons::Icon icon(icons::Symbol s) {
    return {icon_font(), s};
}

ListItem placed(ListItemSpec spec, double width = 360.0) {
    ListItem item(std::move(spec), fonts());
    const layout::Size s = item.measure({0.0, width, 0.0, layout::unbounded});
    item.arrange({0.0, 0.0, s.width, s.height});
    return item;
}

std::vector<ListItemSpec> many(std::size_t n) {
    std::vector<ListItemSpec> items;
    items.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        items.push_back({"Row " + std::to_string(i + 1), i % 3 == 0 ? "Supporting text" : ""});
    }
    return items;
}

input::KeyEvent key(input::Key k) {
    return {k};
}

}

TEST_CASE("list tokens follow ListTokens and the interactive ListItem") {
    const auto& t = tokens::list_tokens;
    CHECK(t.leading_space == 16.0);
    CHECK(t.trailing_space == 16.0);
    CHECK(t.top_space == 10.0);
    CHECK(t.bottom_space == 10.0);
    CHECK(t.between_space == 12.0);
    CHECK(t.one_line_height == 56.0);
    CHECK(t.two_line_height == 72.0);
    CHECK(t.three_line_height == 88.0);
    CHECK(t.top_align_breakpoint == 60.0);
    CHECK(t.headline == tokens::TypeRole::body_large);
    CHECK(t.supporting == tokens::TypeRole::body_medium);
    CHECK(t.overline == tokens::TypeRole::label_small);
    CHECK(t.selected_container == color::Role::secondary_container);
    CHECK(t.corner == tokens::CornerSize::extra_small);
    CHECK(t.hovered_corner == tokens::CornerSize::medium);
    CHECK(t.selected_corner == tokens::CornerSize::large);
}

TEST_CASE("a list item is one, two or three lines tall, at least 56, 72 or 88 dp") {
    CHECK(placed({"Headline"}).lines() == ListLines::one);
    CHECK(placed({"Headline"}).bounds().height == 56.0);
    CHECK(placed({"Headline", "Supporting"}).lines() == ListLines::two);
    CHECK(placed({"Headline", "Supporting"}).bounds().height == 72.0);
    CHECK(placed({"Headline", "Supporting", "Overline"}).lines() == ListLines::three);
    CHECK(placed({"Headline", "Supporting", "Overline"}).bounds().height >= 88.0);
    const ListItem wrapped =
        placed({"Headline", "Supporting text long enough to wrap onto a second line in a narrow item, and then some more words"}, 240.0);
    CHECK(wrapped.lines() == ListLines::three);
    CHECK(wrapped.bounds().height >= 88.0);
}

TEST_CASE("list item slots sit 16 dp from the edges with 12 dp before and after the text") {
    ListItemSpec spec{"Wi-Fi"};
    spec.leading = ListSlot::of_icon(icon(res::icons::home));
    spec.trailing = ListSlot::switch_control();
    const ListItem item = placed(spec);
    CHECK(item.leading_rect().x == 16.0);
    CHECK(item.leading_rect().width == 24.0);
    CHECK(item.leading_rect().y == doctest::Approx(16.0));
    CHECK(item.trailing_rect().x + item.trailing_rect().width == 360.0 - 16.0);
    CHECK(item.trailing_rect().width == 52.0);
    ListItemSpec three{"Headline", "Supporting", "Overline"};
    three.leading = ListSlot::of_avatar(std::string("A"));
    const ListItem top = placed(three);
    CHECK(top.leading_rect().y == doctest::Approx(10.0));
}

TEST_CASE("a toggle list item flips its checkbox and takes the selected colors and shape") {
    ListItemSpec spec{"Notifications"};
    spec.trailing = ListSlot::checkbox();
    spec.action = ListItemAction::toggle;
    ListItem item = placed(spec);
    CHECK(item.corner() == 4.0);
    widgets::ListItemResponse r;
    item.handle({input::PointerAction::down, 100.0, 28.0});
    r = item.handle({input::PointerAction::up, 100.0, 28.0});
    CHECK(r.clicked);
    CHECK(item.selected());
    CHECK(item.accessible().role == widgets::AccessibleRole::checkbox);
    CHECK(item.accessible().state.toggle == widgets::AccessibleToggle::on);
    item.handle({input::PointerAction::leave, 0.0, 0.0});
    settle(item);
    CHECK(item.corner() == doctest::Approx(16.0));
    thorvg::Canvas canvas(360, 56);
    canvas.clear(0xffffffff);
    item.paint(canvas, scheme());
    CHECK(canvas.pixel(180, 4) == scheme()[color::Role::secondary_container]);
}

TEST_CASE("an interactive list item morphs to 12 dp corners on hover and 16 dp when pressed") {
    ListItem item = placed({"Inbox"});
    item.handle({input::PointerAction::move, 50.0, 20.0});
    settle(item);
    CHECK(item.corner() == doctest::Approx(12.0));
    item.handle({input::PointerAction::down, 50.0, 20.0});
    settle(item);
    CHECK(item.corner() == doctest::Approx(16.0));
    item.handle({input::PointerAction::up, 50.0, 20.0});
    item.handle({input::PointerAction::leave, 0.0, 0.0});
    settle(item);
    CHECK(item.corner() == doctest::Approx(4.0));
}

TEST_CASE("a list item without an action takes no input, and a disabled one paints on-surface at 38 %") {
    ListItemSpec spec{"Static"};
    spec.action = ListItemAction::none;
    ListItem item = placed(spec);
    CHECK_FALSE(item.can_focus());
    CHECK(item.accessible().role == widgets::AccessibleRole::list_item);
    item.handle({input::PointerAction::down, 50.0, 20.0});
    CHECK_FALSE(item.handle({input::PointerAction::up, 50.0, 20.0}).clicked);
    ListItemSpec off{"Off"};
    off.enabled = false;
    ListItem disabled = placed(off);
    CHECK_FALSE(disabled.can_focus());
    CHECK_FALSE(disabled.accessible().state.enabled);
}

TEST_CASE("a selectable list item is a radio button and stays selected") {
    ListItemSpec spec{"Large"};
    spec.leading = ListSlot::radio_button();
    spec.action = ListItemAction::select;
    ListItem item = placed(spec);
    item.set_focused(true);
    CHECK(item.handle(key(input::Key::space)).clicked);
    CHECK(item.selected());
    CHECK(item.handle(key(input::Key::enter)).clicked);
    CHECK(item.selected());
    CHECK(item.accessible().role == widgets::AccessibleRole::radio_button);
    CHECK(item.accessible().state.selected == true);
}

TEST_CASE("a list view of 10,000 rows measures and paints only the visible rows and a few more") {
    ListView list({many(10000), false, "Rows"}, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 600.0});
    const std::size_t bound = 600 / 56 + 2 + 2 * tokens::list_tokens.overscan_rows + 1;
    CHECK(list.stats().measured_rows <= bound);
    CHECK(list.stats().live_rows <= bound);
    thorvg::Canvas canvas(360, 600);
    list.paint(canvas, scheme());
    CHECK(list.stats().painted_rows <= bound);
    CHECK(list.content_height() > 10000 * 56.0);
    for (int i = 0; i < 20; ++i) {
        list.handle(input::WheelEvent{100.0, 300.0, 0.0, 3.0});
        list.step(frame_seconds);
        CHECK(list.stats().measured_rows <= bound);
        CHECK(list.stats().live_rows <= bound + 1);
    }
    settle(list);
    CHECK(list.scroll_area().offset() > 1000.0);
    CHECK(list.stats().live_rows <= bound);
    const auto [from, to] = list.visible_rows();
    CHECK(from > 10);
    CHECK(to - from <= bound);
    list.step(frame_seconds);
    CHECK(list.stats().measured_rows == 0);
}

TEST_CASE("a list view is one Tab stop: arrows, Home, End and Page keys move the focus and scroll it into view") {
    std::vector<ListItemSpec> items = many(200);
    items[1].enabled = false;
    ListView list({items, false, "Rows"}, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 300.0});
    widgets::FocusManager focus;
    focus.add(list);
    CHECK(focus.move_focus(widgets::FocusDirection::next));
    CHECK(list.focused_row() == 0u);
    CHECK(list.handle(key(input::Key::down)).consumed);
    CHECK(list.focused_row() == 2u);
    list.handle(key(input::Key::up));
    CHECK(list.focused_row() == 0u);
    list.handle(key(input::Key::up));
    CHECK(list.focused_row() == 199u);
    settle(list);
    CHECK(list.scroll_area().offset() == doctest::Approx(list.scroll_area().max_offset()));
    list.handle(key(input::Key::home));
    CHECK(list.focused_row() == 0u);
    list.handle(key(input::Key::page_down));
    CHECK(*list.focused_row() > 2u);
    settle(list);
    const render::Rect row = list.row_rect(*list.focused_row());
    CHECK(row.y >= -0.5);
    CHECK(row.y + row.height <= 300.5);
    const widgets::ListViewResponse r = list.handle(key(input::Key::enter));
    CHECK(r.clicked == list.focused_row());
    const widgets::AccessibleNode node = list.accessible();
    CHECK(node.role == widgets::AccessibleRole::list);
    CHECK_FALSE(node.children.empty());
    CHECK(node.children.size() < 20);
}

TEST_CASE("a list view keeps checked state in its data when rows leave and come back") {
    std::vector<ListItemSpec> items = many(100);
    for (auto& i : items) {
        i.action = ListItemAction::toggle;
        i.trailing = ListSlot::checkbox();
    }
    ListView list({items}, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 300.0});
    const render::Rect first = list.row_rect(0);
    list.handle({input::PointerAction::down, 100.0, first.y + 20.0});
    const widgets::ListViewResponse r = list.handle({input::PointerAction::up, 100.0, first.y + 20.0});
    CHECK(r.clicked == 0u);
    CHECK(list.item(0).selected);
    list.handle({input::PointerAction::leave, 0.0, 0.0});
    list.scroll_area().jump_to(3000.0);
    for (int i = 0; i < 30; ++i) {
        list.step(frame_seconds);
    }
    list.scroll_area().jump_to(0.0);
    list.step(frame_seconds);
    CHECK(list.item(0).selected);
    CHECK(list.accessible().children[0].state.toggle == widgets::AccessibleToggle::on);
}

TEST_CASE("a single-selection list view keeps one row selected") {
    std::vector<ListItemSpec> items;
    for (const char* s : {"Small", "Medium", "Large"}) {
        ListItemSpec spec{s};
        spec.action = ListItemAction::select;
        spec.leading = ListSlot::radio_button();
        items.push_back(spec);
    }
    items[0].selected = true;
    ListView list({items, true}, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 300.0});
    list.set_focused(true);
    list.handle(key(input::Key::down));
    list.handle(key(input::Key::space));
    CHECK_FALSE(list.item(0).selected);
    CHECK(list.item(1).selected);
}

TEST_CASE("list items and list views satisfy the widget concepts") {
    static_assert(widgets::Measurable<ListItem>);
    static_assert(widgets::FocusGated<ListItem>);
    static_assert(widgets::Disableable<ListItem>);
    static_assert(widgets::Accessible<ListItem>);
    static_assert(widgets::Measurable<ListView>);
    static_assert(widgets::FocusGated<ListView>);
    static_assert(widgets::Disableable<ListView>);
    static_assert(widgets::Accessible<ListView>);
    widgets::WidgetSet set;
    auto& list = set.emplace<ListView>(widgets::ListViewOptions{many(10)}, fonts());
    list.set_viewport({0.0, 0.0, 300.0, 200.0});
    set.set_enabled(false);
    CHECK_FALSE(list.can_focus());
}

TEST_CASE("a list view's top inset leaves room above the first row, as content padding under an app bar does") {
    widgets::ListViewOptions o{many(50)};
    o.top_inset = 112.0;
    ListView list(o, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 400.0});
    CHECK(list.row_rect(0).y == doctest::Approx(112.0));
    CHECK(list.content_height() > 112.0 + 50 * 56.0);
    CHECK(list.row_at(10.0, 50.0) == 0u);
}

TEST_CASE("single selection leaves toggle rows alone") {
    std::vector<ListItemSpec> items{{"Toggle"}, {"Pick"}};
    items[0].action = ListItemAction::toggle;
    items[0].selected = true;
    items[1].action = ListItemAction::select;
    ListView list({items, true}, fonts());
    list.set_viewport({0.0, 0.0, 360.0, 300.0});
    list.set_focused(true);
    list.handle(key(input::Key::down));
    list.handle(key(input::Key::space));
    CHECK(list.item(0).selected);
    CHECK(list.item(1).selected);
}
