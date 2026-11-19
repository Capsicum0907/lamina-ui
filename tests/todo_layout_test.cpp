#include <doctest/doctest.h>

#include "todo/layout.hpp"

using namespace lamina::examples::todo;

TEST_CASE("a narrow window gives the column its whole width less the margins") {
    TodoLayout layout;
    const LayoutSettings& s = layout.settings();
    const Column c = layout.column(400.0);
    CHECK(c.x == doctest::Approx(s.margin));
    CHECK(c.width == doctest::Approx(400.0 - 2.0 * s.margin));
    CHECK(c.text_x == doctest::Approx(s.margin + s.check_size + s.text_gap));
    CHECK(c.remove_x + s.remove_size == doctest::Approx(400.0 - s.margin));
    CHECK(c.text_x + c.text_width + s.remove_gap == doctest::Approx(c.remove_x));
}

TEST_CASE("a wide window keeps the column at its maximum width, centered") {
    TodoLayout layout;
    const LayoutSettings& s = layout.settings();
    const Column c = layout.column(1600.0);
    CHECK(c.width == doctest::Approx(s.max_column_width));
    CHECK(c.x + c.width / 2.0 == doctest::Approx(800.0));
    const TodoFrame f = layout.place(1600.0, 900.0, {});
    CHECK(f.field.x == doctest::Approx(c.x));
    CHECK(f.add.x + f.add.width == doctest::Approx(c.x + c.width));
    CHECK(f.field.x + f.field.width + s.add_gap == doctest::Approx(f.add.x));
}

TEST_CASE("the column width follows the window continuously") {
    TodoLayout layout;
    double previous = layout.column(300.0).width;
    for (double w = 301.0; w < 1000.0; w += 1.0) {
        const double now = layout.column(w).width;
        CHECK(now >= previous);
        CHECK(now - previous <= 1.0 + 1e-9);
        previous = now;
    }
}

TEST_CASE("the list scrolls in a viewport below the input row, across the whole window") {
    TodoLayout layout;
    const LayoutSettings& s = layout.settings();
    const TodoFrame f = layout.place(500.0, 700.0, {});
    CHECK(f.field.y == doctest::Approx(s.margin));
    CHECK(f.field.height == doctest::Approx(s.field_height));
    CHECK(f.add.y + f.add.height / 2.0 == doctest::Approx(f.field.y + f.field.height / 2.0));
    CHECK(f.viewport.x == 0.0);
    CHECK(f.viewport.width == 500.0);
    CHECK(f.viewport.y == doctest::Approx(f.field.y + f.field.height + s.list_gap));
    CHECK(f.viewport.y + f.viewport.height == doctest::Approx(700.0));
    CHECK_FALSE(f.header);
}

TEST_CASE("rows stack in order, then the completed header and the completed rows") {
    TodoLayout layout;
    const LayoutSettings& s = layout.settings();
    const TodoFrame f = layout.place(400.0, 700.0, {{48.0, 72.0, 48.0}, {48.0, 96.0}, true});
    REQUIRE(f.active.size() == 3);
    CHECK(f.active[0].y == doctest::Approx(s.list_top_padding));
    CHECK(f.active[1].y == doctest::Approx(s.list_top_padding + 48.0));
    CHECK(f.active[2].y == doctest::Approx(s.list_top_padding + 120.0));
    CHECK(f.active[1].height == 72.0);
    REQUIRE(f.header);
    CHECK(f.header->y == doctest::Approx(s.list_top_padding + 168.0 + s.section_gap));
    CHECK(f.header->height == doctest::Approx(s.header_height));
    REQUIRE(f.completed.size() == 2);
    CHECK(f.completed[0].y == doctest::Approx(f.header->y + s.header_height));
    CHECK(f.completed[1].y == doctest::Approx(f.completed[0].y + 48.0));
    CHECK(f.completed[1].shown);
    CHECK(f.content_height == doctest::Approx(f.completed[1].y + 96.0 + s.list_end_padding));
}

TEST_CASE("collapsed, the completed rows fold to the header and leave the content height") {
    TodoLayout layout;
    const LayoutSettings& s = layout.settings();
    const TodoFrame f = layout.place(400.0, 700.0, {{48.0}, {48.0, 96.0}, false});
    REQUIRE(f.header);
    const double bottom = f.header->y + s.header_height;
    for (const RowPlace& r : f.completed) {
        CHECK(r.y == doctest::Approx(bottom));
        CHECK(r.height == 0.0);
        CHECK_FALSE(r.shown);
    }
    CHECK(f.content_height == doctest::Approx(bottom + s.list_end_padding));
}

TEST_CASE("with no open rows the header starts the list") {
    TodoLayout layout;
    const TodoFrame f = layout.place(400.0, 700.0, {{}, {48.0}, true});
    REQUIRE(f.header);
    CHECK(f.header->y == doctest::Approx(layout.settings().list_top_padding));
}
