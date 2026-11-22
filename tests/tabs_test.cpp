#include <doctest/doctest.h>

#include <cmath>
#include <string>

#include "lamina/tokens/scroll_area.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::TabStyle;

namespace {

widgets::TabRow make(TabStyle style, bool icons = false) {
    std::vector<widgets::TabItem> items;
    for (const char* label : {"One", "Two", "Three"}) {
        widgets::TabItem item{label};
        if (icons) {
            item.icon = icons::Icon{icon_font(), res::icons::home};
        }
        items.push_back(item);
    }
    widgets::TabRow row({items, style}, fonts());
    row.set_width(300.0);
    return row;
}

}

TEST_CASE("tab tokens follow the navigation tab tokens and Tab.kt") {
    const auto& t = tokens::tab_tokens;
    CHECK(t.height == 48.0);
    CHECK(t.icon_and_label_height == 72.0);
    CHECK(t.horizontal_text_padding == 16.0);
    CHECK(t.label == tokens::TypeRole::title_small);
    CHECK(t.divider == color::Role::outline_variant);
    const auto& primary = tokens::tab_style(TabStyle::primary);
    CHECK(primary.active_content == color::Role::primary);
    CHECK(primary.inactive_content == color::Role::on_surface_variant);
    CHECK(primary.indicator_height == 3.0);
    CHECK(primary.indicator_matches_content);
    const auto& secondary = tokens::tab_style(TabStyle::secondary);
    CHECK(secondary.active_content == color::Role::on_surface);
    CHECK_FALSE(secondary.indicator_matches_content);
}

TEST_CASE("a tab row is 48 dp high, or 72 dp with icons above the labels") {
    CHECK(make(TabStyle::primary).height() == 48.0);
    CHECK(make(TabStyle::primary, true).height() == 72.0);
    std::vector<widgets::TabItem> items{{"A", icons::Icon{icon_font(), res::icons::home}}};
    widgets::TabRow leading({items, TabStyle::primary, 0, widgets::TabIconPosition::leading}, fonts());
    CHECK(leading.height() == 48.0);
}

TEST_CASE("clicking a tab selects it and the indicator slides there with a spring") {
    auto row = make(TabStyle::secondary);
    CHECK(row.selected() == 0);
    CHECK(row.indicator_rect().x == doctest::Approx(0.0));
    CHECK(row.indicator_rect().width == doctest::Approx(100.0));
    CHECK(click(row, 250.0, 24.0));
    CHECK(row.selected() == 2);
    CHECK(row.animating());
    row.step(1.0 / 60.0);
    const double moving = row.indicator_rect().x;
    CHECK(moving > 0.0);
    CHECK(moving < 200.0);
    settle(row);
    CHECK(row.indicator_rect().x == doctest::Approx(200.0));
    CHECK(row.selection(2) == doctest::Approx(1.0));
    CHECK(row.selection(0) == doctest::Approx(0.0));
    CHECK_FALSE(click(row, 250.0, 24.0));
    CHECK_FALSE(click(row, 150.0, 100.0));
    CHECK(row.selected() == 2);
}

TEST_CASE("the primary indicator matches the label width, at least 24 dp, centered on the tab") {
    auto row = make(TabStyle::primary);
    const auto label = text::layout_line(fonts(), "Two", tokens::text_style(tokens::type_style(tokens::TypeRole::title_small)));
    row.select(1);
    settle(row);
    const render::Rect r = row.indicator_rect();
    CHECK(r.width == doctest::Approx(std::max(label.width, 24.0)));
    CHECK(r.x + r.width / 2.0 == doctest::Approx(150.0));
    CHECK(r.y == doctest::Approx(45.0));
}

TEST_CASE("the first layout places the indicator without animating") {
    std::vector<widgets::TabItem> items{{"A"}, {"B"}};
    widgets::TabRow row({items, TabStyle::secondary, 1}, fonts());
    row.set_width(200.0);
    CHECK_FALSE(row.animating());
    CHECK(row.indicator_rect().x == doctest::Approx(100.0));
}

TEST_CASE("the tab row paints the indicator, the divider and the selected label color") {
    auto row = make(TabStyle::secondary);
    thorvg::Canvas canvas(300, 48);
    canvas.clear(0xffffffff);
    row.paint(canvas, scheme());
    CHECK(canvas.pixel(50, 46) == scheme()[color::Role::primary]);
    CHECK(canvas.pixel(150, 47) == scheme()[color::Role::outline_variant]);
    CHECK(canvas.pixel(150, 4) == scheme()[color::Role::surface]);
}

namespace {

widgets::TabRow make_scrollable(std::size_t count, double width, std::size_t selected = 0) {
    std::vector<widgets::TabItem> items;
    for (std::size_t i = 0; i < count; ++i) {
        items.push_back({"Tab " + std::to_string(i + 1)});
    }
    widgets::TabRowOptions options{items, TabStyle::secondary, selected};
    options.scrollable = true;
    widgets::TabRow row(options, fonts());
    row.set_width(width);
    return row;
}

}

TEST_CASE("scrollable tabs take the TabRow.kt minimum width and edge padding") {
    CHECK(tokens::tab_tokens.scrollable_min_tab_width == 90.0);
    CHECK(tokens::tab_tokens.scrollable_edge_padding == 52.0);
    auto row = make_scrollable(10, 300.0);
    CHECK(row.tab_rect(0).x == doctest::Approx(52.0));
    CHECK(row.tab_rect(0).width == doctest::Approx(90.0));
    CHECK(row.tab_rect(1).x == doctest::Approx(142.0));
    CHECK(row.scroll_extent() == doctest::Approx(52.0 * 2.0 + 90.0 * 10.0));
    CHECK(row.max_scroll() == doctest::Approx(row.scroll_extent() - 300.0));
}

TEST_CASE("scrollable tabs that fit do not scroll and do not stretch") {
    auto row = make_scrollable(2, 600.0);
    CHECK(row.max_scroll() == 0.0);
    CHECK(row.tab_rect(1).x + row.tab_rect(1).width == doctest::Approx(52.0 + 180.0));
}

TEST_CASE("the first layout of scrollable tabs centres the selected tab without animating") {
    auto row = make_scrollable(10, 300.0, 5);
    CHECK_FALSE(row.animating());
    const render::Rect r = row.tab_rect(5);
    CHECK(r.x + r.width / 2.0 == doctest::Approx(150.0));
    const render::Rect i = row.indicator_rect();
    CHECK(i.x == doctest::Approx(r.x));
}

TEST_CASE("selecting a scrollable tab scrolls it toward the centre on a spring, with the indicator following continuously") {
    auto row = make_scrollable(10, 300.0);
    CHECK(row.scroll_offset() == 0.0);
    row.select(6);
    const double travel = row.scroll_offset() == 0.0 ? 52.0 + 90.0 * 6.5 - 150.0 : 0.0;
    double last_scroll = row.scroll_offset();
    double last_indicator = row.indicator_rect().x + row.scroll_offset();
    int moving = 0;
    for (int f = 0; f < settle_frames; ++f) {
        row.step(frame_seconds);
        const double s = row.scroll_offset();
        const double i = row.indicator_rect().x + s;
        CHECK(std::abs(s - last_scroll) < 0.2 * travel);
        CHECK(std::abs(i - last_indicator) < 0.2 * travel);
        moving += s != last_scroll ? 1 : 0;
        last_scroll = s;
        last_indicator = i;
    }
    CHECK(moving > 6);
    const render::Rect r = row.tab_rect(6);
    CHECK(r.x + r.width / 2.0 == doctest::Approx(150.0).epsilon(0.001));
    row.select(9);
    settle(row);
    CHECK(row.scroll_offset() == doctest::Approx(row.max_scroll()));
}

TEST_CASE("tabs scrolled out of view cannot be clicked and report only their visible part") {
    auto row = make_scrollable(10, 300.0);
    const render::Rect hidden = row.tab_rect(5);
    CHECK(hidden.x > 300.0);
    CHECK_FALSE(click(row, hidden.x + 10.0, 24.0));
    CHECK(row.selected() == 0);
    const auto node = row.accessible();
    CHECK(node.children[5].bounds.width == 0.0);
    const render::Rect cut = row.tab_rect(2);
    CHECK(node.children[2].bounds.x + node.children[2].bounds.width == doctest::Approx(300.0));
    CHECK(node.children[2].bounds.x == doctest::Approx(cut.x));
}

TEST_CASE("the wheel scrolls a scrollable tab row and stops at its ends") {
    auto row = make_scrollable(10, 300.0);
    CHECK(row.handle(input::WheelEvent{150.0, 24.0, 0.0, 1.0}));
    settle(row);
    CHECK(row.scroll_offset() == doctest::Approx(tokens::scroll_area_tokens.wheel_step));
    CHECK_FALSE(row.handle(input::WheelEvent{150.0, 100.0, 0.0, 1.0}));
    for (int i = 0; i < 40; ++i) {
        row.handle(input::WheelEvent{150.0, 24.0, 1.0, 0.0});
    }
    settle(row);
    CHECK(row.scroll_offset() == doctest::Approx(row.max_scroll()));
    auto fixed = make(TabStyle::secondary);
    CHECK_FALSE(fixed.handle(input::WheelEvent{150.0, 24.0, 0.0, 1.0}));
}

TEST_CASE("narrowing a scrolled tab row keeps the offset within its range without a jump") {
    auto row = make_scrollable(10, 300.0);
    row.select(9);
    settle(row);
    double last = row.tab_rect(9).x;
    for (double w = 300.0; w <= 1100.0; w += 20.0) {
        row.set_width(w);
        row.step(frame_seconds);
        CHECK(row.scroll_offset() <= row.max_scroll() + 1e-9);
        CHECK(std::abs(row.tab_rect(9).x - last) < 40.0);
        last = row.tab_rect(9).x;
    }
    settle(row);
    CHECK(row.scroll_offset() == 0.0);
}
