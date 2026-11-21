#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/table.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

const auto& t = tokens::table_tokens;

std::string name_of(std::size_t row) {
    static const std::vector<std::string> names{"delta", "alpha", "echo", "charlie", "bravo"};
    return names[row % names.size()] + "-" + std::to_string(row);
}

widgets::Table make(std::size_t rows, double width = 600.0, double height = 400.0) {
    widgets::Table table({.columns = {{.title = "Name", .width = 200.0},
                                      {.title = "Size", .width = 120.0, .align = widgets::ColumnAlign::end,
                                       .less = [](std::size_t a, std::size_t b) { return (a * 37) % 101 < (b * 37) % 101; }},
                                      {.title = "Kind", .width = 160.0, .sortable = false, .resizable = false}},
                          .row_count = rows,
                          .cell = [](std::size_t r, std::size_t c) {
                              if (c == 0) {
                                  return name_of(r);
                              }
                              if (c == 1) {
                                  return std::to_string((r * 37) % 101) + " KB";
                              }
                              return std::string(r % 2 ? "Image" : "Text");
                          },
                          .accessible_name = "Files",
                          .icon_font = icon_font()},
                         fonts());
    table.arrange({0.0, 0.0, width, height});
    return table;
}

render::Point centre(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

void click(widgets::Table& table, render::Point p, bool shift = false, bool ctrl = false, int clicks = 1) {
    table.handle(input::PointerEvent{PointerAction::down, p.x, p.y, clicks, input::PointerKind::mouse, 0.0, shift, ctrl});
    table.handle(input::PointerEvent{PointerAction::up, p.x, p.y, 0, input::PointerKind::mouse, 0.0, shift, ctrl});
}

}

TEST_CASE("table tokens follow MDC's data table where it has a value") {
    CHECK(t.row_height == 52.0);
    CHECK(t.header_height == 56.0);
    CHECK(t.cell_leading_padding == 16.0);
    CHECK(t.cell_trailing_padding == 16.0);
    CHECK(t.sort_icon_gap == 4.0);
    CHECK(t.divider_thickness == 1.0);
    CHECK(tokens::density_tokens.table_row.at(tokens::Density{-4}) == 36.0);
    CHECK(tokens::density_tokens.table_header.at(tokens::Density{-4}) == 40.0);
}

TEST_CASE("a table needs a name, a cell function and an icon font for sortable columns") {
    CHECK_THROWS_AS(widgets::Table({.columns = {{.title = "A"}}, .cell = [](std::size_t, std::size_t) { return std::string(); }}, fonts()),
                    std::invalid_argument);
    CHECK_THROWS_AS(widgets::Table({.columns = {{.title = "A"}}, .accessible_name = "T"}, fonts()), std::invalid_argument);
    CHECK_THROWS_AS(
        widgets::Table({.columns = {{.title = "A"}}, .cell = [](std::size_t, std::size_t) { return std::string(); }, .accessible_name = "T"},
                       fonts()),
        std::invalid_argument);
}

TEST_CASE("the header stays on top and rows sit below it") {
    widgets::Table table = make(100);
    CHECK(table.header_rect().height == 56.0);
    CHECK(table.viewport().y == 56.0);
    CHECK(table.viewport().height == 344.0);
    CHECK(table.row_rect(0).y == 56.0);
    CHECK(table.row_rect(3).y == 56.0 + 3.0 * 52.0);
    CHECK(table.content_width() == 480.0);
    CHECK(table.scroll_area().content_height() == 5200.0);
    table.scroll_area().jump_to(1040.0);
    CHECK(table.header_rect().y == 0.0);
    CHECK(table.row_at(10.0, 60.0) == 20u);
}

TEST_CASE("clicking a header sorts ascending, again descending, and the arrow follows") {
    widgets::Table table = make(10);
    click(table, centre(table.column_header_rect(0)));
    REQUIRE(table.sort());
    CHECK(*table.sort() == widgets::TableSort{0, widgets::SortDirection::ascending});
    CHECK(name_of(table.model_row(0)) == "alpha-1");
    CHECK(name_of(table.model_row(1)) == "alpha-6");
    CHECK(name_of(table.model_row(9)) == "echo-7");
    click(table, centre(table.column_header_rect(0)));
    CHECK(table.sort()->direction == widgets::SortDirection::descending);
    CHECK(name_of(table.model_row(0)) == "echo-7");
    click(table, centre(table.column_header_rect(1)));
    CHECK(table.sort()->column == 1u);
    for (std::size_t v = 1; v < 10; ++v) {
        CHECK((table.model_row(v - 1) * 37) % 101 <= (table.model_row(v) * 37) % 101);
    }
    click(table, centre(table.column_header_rect(2)));
    CHECK(table.sort()->column == 1u);
    const widgets::AccessibleNode n = table.accessible();
    CHECK(n.children[0].children[1].value == "Ascending");
    CHECK(n.children[0].children[0].value.empty());
}

TEST_CASE("sorting keeps the selection and the focus on the same rows") {
    widgets::Table table = make(10);
    click(table, centre(table.row_rect(3)));
    CHECK(table.is_selected(3));
    table.sort_by(0, widgets::SortDirection::ascending);
    CHECK(table.is_selected(3));
    CHECK(table.selected_count() == 1);
    CHECK(table.model_row(*table.focused_row()) == 3u);
    table.clear_sort();
    CHECK(table.model_row(3) == 3u);
}

TEST_CASE("dragging a header divider resizes the column down to its minimum, and the cursor shows it") {
    widgets::Table table = make(10);
    const render::Rect name = table.column_header_rect(0);
    const double edge = name.x + name.width;
    table.handle(input::PointerEvent{PointerAction::move, edge - 2.0, 20.0});
    CHECK(table.divider_at(edge - 2.0, 20.0) == 0u);
    CHECK(table.cursor() == input::Cursor::resize_horizontal);
    table.handle(input::PointerEvent{PointerAction::down, edge - 2.0, 20.0, 1});
    table.handle(input::PointerEvent{PointerAction::move, edge + 58.0, 20.0});
    CHECK(table.column_width(0) == doctest::Approx(260.0));
    CHECK(table.column_header_rect(1).x == doctest::Approx(260.0));
    table.handle(input::PointerEvent{PointerAction::move, edge - 400.0, 20.0});
    CHECK(table.column_width(0) == t.default_min_column_width);
    const widgets::TableResponse r = table.handle(input::PointerEvent{PointerAction::up, edge - 400.0, 20.0});
    CHECK(r.resized == 0u);
    CHECK_FALSE(table.sort());
    const render::Rect kind = table.column_header_rect(2);
    CHECK_FALSE(table.divider_at(kind.x + kind.width - 1.0, 20.0));
    table.handle(input::PointerEvent{PointerAction::move, 300.0, 200.0});
    CHECK_FALSE(table.cursor());
}

TEST_CASE("a wider table than its viewport scrolls sideways and the header scrolls with it") {
    widgets::Table table = make(10, 300.0, 400.0);
    CHECK(table.scroll_area().max_offset_x() == doctest::Approx(180.0));
    table.handle(input::WheelEvent{100.0, 200.0, 0.0, 1.0, true});
    settle(table);
    CHECK(table.scroll_area().offset_x() > 0.0);
    CHECK(table.column_header_rect(0).x == doctest::Approx(-table.scroll_area().offset_x()));
    CHECK(table.cell_rect(0, 1).x == doctest::Approx(table.column_header_rect(1).x));
}

TEST_CASE("click selects one row, Ctrl+click toggles, Shift+click selects a range from the anchor") {
    widgets::Table table = make(20, 600.0, 700.0);
    click(table, centre(table.row_rect(2)));
    CHECK(table.selected_rows() == std::vector<std::size_t>{2});
    click(table, centre(table.row_rect(5)), false, true);
    CHECK(table.selected_rows() == std::vector<std::size_t>{2, 5});
    click(table, centre(table.row_rect(2)), false, true);
    CHECK(table.selected_rows() == std::vector<std::size_t>{5});
    click(table, centre(table.row_rect(2)), true, false);
    CHECK(table.selected_rows() == std::vector<std::size_t>{2});
    click(table, centre(table.row_rect(4)));
    click(table, centre(table.row_rect(1)), true, false);
    CHECK(table.selected_rows() == std::vector<std::size_t>{1, 2, 3, 4});
    click(table, centre(table.row_rect(6)), false, true);
    click(table, centre(table.row_rect(8)), true, true);
    CHECK(table.selected_rows() == std::vector<std::size_t>{1, 2, 3, 4, 6, 7, 8});
    widgets::TableResponse r = table.handle(input::PointerEvent{PointerAction::down, 10.0, table.row_rect(3).y + 5.0, 2});
    CHECK(r.activated == 3u);
}

TEST_CASE("keyboard: arrows move and select, Shift extends, Ctrl moves the focus only, Ctrl+Space toggles, Ctrl+A selects all") {
    widgets::Table table = make(30);
    widgets::FocusManager focus;
    focus.add(table);
    focus.handle(input::KeyEvent{Key::tab, false});
    REQUIRE(table.focused());
    table.handle(input::KeyEvent{Key::down, false});
    CHECK(table.selected_rows() == std::vector<std::size_t>{1});
    table.handle(input::KeyEvent{Key::down, true});
    table.handle(input::KeyEvent{Key::down, true});
    CHECK(table.selected_rows() == std::vector<std::size_t>{1, 2, 3});
    table.handle(input::KeyEvent{Key::down, false, true});
    table.handle(input::KeyEvent{Key::down, false, true});
    CHECK(table.focused_row() == 5u);
    CHECK(table.selected_count() == 3);
    table.handle(input::KeyEvent{Key::space, false, true});
    CHECK(table.selected_rows() == std::vector<std::size_t>{1, 2, 3, 5});
    table.handle(input::KeyEvent{Key::end, false});
    CHECK(table.selected_rows() == std::vector<std::size_t>{29});
    CHECK(table.scroll_area().target_offset() == doctest::Approx(30.0 * 52.0 - 344.0));
    table.handle(input::KeyEvent{Key::page_up, false});
    CHECK(table.focused_row() == 24u);
    table.handle(input::KeyEvent{Key::home, false});
    CHECK(table.focused_row() == 0u);
    CHECK(focus.claims_key(input::KeyEvent{Key::a, false, true}));
    table.handle(input::KeyEvent{Key::a, false, true});
    CHECK(table.selected_count() == 30);
    const widgets::TableResponse r = table.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.activated == 0u);
    CHECK_FALSE(table.handle(input::KeyEvent{Key::left, false}).consumed);
}

TEST_CASE("a right click selects the row under it unless it is already selected") {
    widgets::Table table = make(20, 600.0, 700.0);
    click(table, centre(table.row_rect(2)));
    click(table, centre(table.row_rect(4)), false, true);
    CHECK(table.handle(input::ContextClick{50.0, table.row_rect(4).y + 10.0}));
    CHECK(table.selected_count() == 2);
    CHECK(table.handle(input::ContextClick{50.0, table.row_rect(7).y + 10.0}));
    CHECK(table.selected_rows() == std::vector<std::size_t>{7});
    CHECK_FALSE(table.handle(input::ContextClick{900.0, 900.0}));
}

TEST_CASE("only the rows in view are laid out and painted, even with a hundred thousand rows") {
    widgets::Table table = make(100000);
    thorvg::Canvas canvas(600, 400);
    table.paint(canvas, scheme());
    const auto [from, to] = table.visible_rows();
    CHECK(from == 0u);
    CHECK(to == 7u + t.overscan_rows);
    CHECK(table.stats().painted_rows == to - from);
    CHECK(table.stats().cached_rows <= to - from);
    table.scroll_area().jump_to(52.0 * 50000.0);
    canvas.clear(0);
    table.paint(canvas, scheme());
    const auto [from2, to2] = table.visible_rows();
    CHECK(from2 == 50000u - t.overscan_rows);
    CHECK(table.stats().painted_rows == to2 - from2);
    CHECK(table.row_at(10.0, 60.0) == 50000u);
    const widgets::AccessibleNode n = table.accessible();
    CHECK(n.children.size() == 1 + 7);
    CHECK(n.children[1].name == name_of(50000));
}

TEST_CASE("the density shortens rows and header") {
    widgets::Table table = make(100);
    table.set_density(tokens::Density{-4});
    CHECK(table.row_height() == 36.0);
    CHECK(table.header_height() == 40.0);
    CHECK(table.viewport().y == 40.0);
    CHECK(table.scroll_area().content_height() == 3600.0);
}

TEST_CASE("a disabled table takes no input and gives up the focus") {
    widgets::Table table = make(10);
    table.set_focused(true);
    table.set_enabled(false);
    CHECK_FALSE(table.focused());
    click(table, centre(table.row_rect(1)));
    CHECK(table.selected_count() == 0);
    CHECK_FALSE(table.handle(input::KeyEvent{Key::down, false}).consumed);
}
