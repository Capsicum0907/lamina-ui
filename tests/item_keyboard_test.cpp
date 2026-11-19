#include <doctest/doctest.h>

#include <vector>

#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/item_focus.hpp"
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

}

static_assert(widgets::Focusable<widgets::TabRow>);
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
