#include <doctest/doctest.h>

#include <memory>

#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/split_pane.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::SplitPane;

static_assert(widgets::Measurable<SplitPane> && widgets::Paintable<SplitPane> && widgets::Steppable<SplitPane> &&
              widgets::HandlesPointer<SplitPane> && widgets::HandlesKeys<SplitPane> && widgets::FocusGated<SplitPane> &&
              widgets::Disableable<SplitPane> && widgets::Cancellable<SplitPane>);

namespace {

constexpr const tokens::SplitPaneTokens& t = tokens::split_pane_tokens;

struct Box {
    render::Rect rect{0.0, 0.0, 0.0, 0.0};
};

layout::NodePtr box(Box& b) {
    return std::make_shared<layout::Leaf>([](const layout::Constraints& c) { return layout::Size{c.min_width, c.min_height}; },
                                          [&b](const render::Rect& r) { b.rect = r; });
}

void place(SplitPane& s, const render::Rect& r) {
    s.measure(layout::Constraints::tight({r.width, r.height}));
    s.arrange(r);
}

input::KeyEvent key(Key k) {
    return {k, false};
}

}

TEST_CASE("a split pane divides its space at a fraction and arranges both children") {
    Box a;
    Box b;
    SplitPane s({.first = box(a), .second = box(b), .first_fraction = 0.25});
    place(s, {0.0, 0.0, 801.0, 400.0});
    CHECK(s.first_size() == doctest::Approx(200.0));
    CHECK(a.rect.width == doctest::Approx(200.0));
    CHECK(b.rect.x == doctest::Approx(201.0));
    CHECK(b.rect.width == doctest::Approx(600.0));
    CHECK(s.divider_rect().width == t.divider_thickness);
}

TEST_CASE("a vertical split pane stacks its children") {
    Box a;
    Box b;
    SplitPane s({.axis = layout::Axis::vertical, .first = box(a), .second = box(b), .first_size = 100.0});
    place(s, {10.0, 20.0, 300.0, 401.0});
    CHECK(a.rect.height == 100.0);
    CHECK(b.rect.y == 20.0 + 101.0);
    CHECK(b.rect.height == 300.0);
}

TEST_CASE("dragging the divider resizes the panes within their minimums and changes the cursor") {
    Box a;
    Box b;
    SplitPane s({.first = box(a), .second = box(b), .first_min = 120.0, .second_min = 200.0, .first_size = 300.0});
    place(s, {0.0, 0.0, 801.0, 400.0});
    CHECK_FALSE(s.cursor().has_value());
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::move, 200.0, 100.0}));
    CHECK(s.handle(input::PointerEvent{PointerAction::down, 302.0, 100.0}) == true);
    CHECK(s.dragging());
    REQUIRE(s.cursor().has_value());
    CHECK(*s.cursor() == input::Cursor::resize_horizontal);
    s.handle(input::PointerEvent{PointerAction::move, 402.0, 100.0});
    place(s, {0.0, 0.0, 801.0, 400.0});
    CHECK(s.first_size() == doctest::Approx(400.0));
    CHECK(a.rect.width == doctest::Approx(400.0));
    s.handle(input::PointerEvent{PointerAction::move, 40.0, 100.0});
    CHECK(s.first_size() == 120.0);
    s.handle(input::PointerEvent{PointerAction::move, 790.0, 100.0});
    CHECK(s.first_size() == 600.0);
    s.handle(input::PointerEvent{PointerAction::up, 790.0, 100.0});
    CHECK_FALSE(s.dragging());
    s.handle(input::PointerEvent{PointerAction::move, 100.0, 100.0});
    CHECK_FALSE(s.cursor().has_value());
}

TEST_CASE("a focused divider moves with the arrow keys, Page keys, Home and End, and Enter collapses and restores the first pane") {
    SplitPane s({.first_min = 100.0, .second_min = 100.0, .first_size = 300.0, .accessible_name = "Sidebar"});
    place(s, {0.0, 0.0, 801.0, 400.0});
    CHECK_FALSE(s.handle(key(Key::right)));
    s.set_focused(true);
    CHECK(s.handle(key(Key::right)));
    CHECK(s.first_size() == 300.0 + t.keyboard_step);
    CHECK(s.handle(key(Key::left)));
    CHECK(s.handle(key(Key::page_down)));
    CHECK(s.first_size() == 300.0 + t.page_step);
    CHECK_FALSE(s.handle(key(Key::down)));
    CHECK(s.handle(key(Key::home)));
    CHECK(s.first_size() == 100.0);
    CHECK(s.handle(key(Key::end)));
    CHECK(s.first_size() == 700.0);
    s.set_first_size(320.0);
    CHECK(s.handle(key(Key::enter)));
    CHECK(s.first_size() == 100.0);
    CHECK(s.handle(key(Key::enter)));
    CHECK(s.first_size() == 320.0);
    const widgets::AccessibleNode n = s.accessible();
    CHECK(n.role == widgets::AccessibleRole::separator);
    CHECK(n.name == "Sidebar");
    REQUIRE(n.state.range.has_value());
    CHECK(n.state.range->value == 320.0);
    CHECK(n.state.range->max == 700.0);
    CHECK(n.state.focused);
}

TEST_CASE("nested split panes each keep their own divider") {
    Box a;
    Box b;
    Box c;
    SplitPane inner({.axis = layout::Axis::vertical, .first = box(b), .second = box(c), .first_fraction = 0.5});
    SplitPane outer({.first = box(a), .second = widgets::node(inner), .first_size = 200.0});
    place(outer, {0.0, 0.0, 801.0, 601.0});
    CHECK(b.rect.x == 201.0);
    CHECK(b.rect.height == doctest::Approx(300.0));
    CHECK(c.rect.y == doctest::Approx(301.0));
    CHECK(inner.handle(input::PointerEvent{PointerAction::down, 500.0, 300.5}));
    inner.handle(input::PointerEvent{PointerAction::move, 500.0, 400.5});
    inner.handle(input::PointerEvent{PointerAction::up, 500.0, 400.5});
    place(outer, {0.0, 0.0, 801.0, 601.0});
    CHECK(b.rect.height == doctest::Approx(400.0));
    CHECK(*inner.cursor() == input::Cursor::resize_vertical);
}

TEST_CASE("a disabled split pane ignores the pointer and the keys") {
    SplitPane s({.first_size = 300.0, .enabled = false});
    place(s, {0.0, 0.0, 801.0, 400.0});
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::down, 300.5, 100.0}));
    s.set_focused(true);
    CHECK_FALSE(s.focused());
    CHECK_FALSE(s.accessible().state.enabled);
}

TEST_CASE("the divider paints a line and shows the drag handle while hovered") {
    thorvg::Canvas canvas(400, 200);
    SplitPane s({.first_size = 200.0});
    place(s, {0.0, 0.0, 401.0, 200.0});
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    const int line = count_color(canvas, scheme()[t.divider]);
    CHECK(line >= 190);
    s.handle(input::PointerEvent{PointerAction::move, 200.0, 100.0});
    settle(s);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[tokens::drag_handle_tokens.color]) > 100);
}
