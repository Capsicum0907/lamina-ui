#include <doctest/doctest.h>

#include "lamina/tokens/tooltip.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/tooltip.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using input::PointerKind;
using widgets::TooltipTrigger;

namespace {

constexpr layout::Size window{600.0, 400.0};

widgets::IconButton anchor_button(double x = 280.0, double y = 200.0) {
    widgets::IconButton b({{icon_font(), res::icons::search}, "Search"});
    b.set_origin({x, y});
    return b;
}

void step_for(widgets::Tooltip& t, double seconds) {
    for (double s = 0.0; s < seconds; s += frame_seconds) {
        t.step(frame_seconds);
    }
}

}

static_assert(widgets::FocusGated<widgets::Tooltip>);
static_assert(widgets::Accessible<widgets::Tooltip>);

TEST_CASE("a plain tooltip is one line of Body S padded 8 by 4, at least 40 by 24") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts());
    t.set_window(window);
    t.show();
    const render::Rect r = t.bounds();
    CHECK(r.height == doctest::Approx(24.0));
    CHECK(r.width >= tokens::tooltip_tokens.min_width);
    CHECK(r.width <= tokens::plain_tooltip_tokens.max_width);
    const render::Rect a = b.bounds();
    CHECK(r.y + r.height + tokens::tooltip_tokens.anchor_spacing == doctest::Approx(a.y));
    CHECK(r.x + r.width / 2.0 == doctest::Approx(a.x + a.width / 2.0));
}

TEST_CASE("the tooltip goes below the anchor when there is no room above, and stays in the window") {
    const layout::Size tooltip{120.0, 24.0};
    const render::Point top = widgets::tooltip_position({10.0, 2.0, 40.0, 40.0}, window, tooltip, widgets::TooltipPosition::above, 4.0);
    CHECK(top.y == doctest::Approx(46.0));
    CHECK(top.x == doctest::Approx(0.0));
    const render::Point bottom =
        widgets::tooltip_position({300.0, 380.0, 40.0, 20.0}, window, tooltip, widgets::TooltipPosition::below, 4.0);
    CHECK(bottom.y == doctest::Approx(380.0 - 24.0 - 4.0));
    const render::Point right =
        widgets::tooltip_position({560.0, 100.0, 40.0, 40.0}, window, tooltip, widgets::TooltipPosition::right, 4.0);
    CHECK(right.x == doctest::Approx(560.0 - 124.0));
    CHECK(right.y == doctest::Approx(108.0));
}

TEST_CASE("a mouse over the anchor shows the tooltip at once, as Compose does, and leaving hides it") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts());
    t.set_window(window);
    t.handle({PointerAction::move, 290.0, 210.0});
    CHECK(t.is_open());
    CHECK(t.trigger() == TooltipTrigger::hover);
    CHECK(t.scale() == doctest::Approx(tokens::tooltip_tokens.closed_scale));
    step_for(t, 0.05);
    CHECK(t.opacity() > 0.0);
    step_for(t, 3.0);
    CHECK(t.is_open());
    CHECK(t.scale() == doctest::Approx(1.0));
    t.handle({PointerAction::move, 10.0, 10.0});
    CHECK_FALSE(t.is_open());
    CHECK(t.visible());
    step_for(t, 1.0);
    CHECK_FALSE(t.visible());
}

TEST_CASE("a hover delay waits before showing and reports the wake time") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts(), {.hover_delay_seconds = 0.5});
    t.set_window(window);
    t.handle({PointerAction::move, 290.0, 210.0});
    CHECK_FALSE(t.is_open());
    REQUIRE(t.next_wake_seconds());
    CHECK(*t.next_wake_seconds() == doctest::Approx(0.5));
    step_for(t, 0.3);
    CHECK_FALSE(t.is_open());
    step_for(t, 0.3);
    CHECK(t.is_open());
}

TEST_CASE("keyboard focus on the anchor shows the tooltip for 1.5 s; losing the focus or Escape hides it") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts());
    t.set_window(window);
    b.set_focused(true);
    t.step(frame_seconds);
    CHECK(t.is_open());
    CHECK(t.trigger() == TooltipTrigger::focus);
    REQUIRE(t.next_wake_seconds());
    step_for(t, 1.6);
    CHECK_FALSE(t.is_open());

    b.set_focused(false);
    t.step(frame_seconds);
    b.set_focused(true);
    t.step(frame_seconds);
    CHECK(t.is_open());
    b.set_focused(false);
    t.step(frame_seconds);
    CHECK_FALSE(t.is_open());

    b.set_focused(true);
    t.step(frame_seconds);
    CHECK(t.handle(input::KeyEvent{Key::escape}).consumed);
    CHECK_FALSE(t.is_open());
}

TEST_CASE("a long touch press shows the tooltip and releasing hides it; the mouse never long-presses") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts());
    t.set_window(window);
    t.handle({PointerAction::down, 290.0, 210.0, 1, PointerKind::touch});
    step_for(t, 0.3);
    CHECK_FALSE(t.is_open());
    step_for(t, 0.3);
    CHECK(t.is_open());
    CHECK(t.trigger() == TooltipTrigger::long_press);
    t.handle({PointerAction::up, 290.0, 210.0, 0, PointerKind::touch});
    CHECK_FALSE(t.is_open());

    t.handle({PointerAction::down, 290.0, 210.0, 1, PointerKind::touch});
    t.handle({PointerAction::up, 290.0, 210.0, 0, PointerKind::touch});
    step_for(t, 1.0);
    CHECK_FALSE(t.is_open());
}

TEST_CASE("a persistent rich tooltip stays after the pointer leaves and reports its action") {
    widgets::Button anchor({"Info"}, fonts());
    anchor.set_origin({100.0, 200.0});
    widgets::Tooltip t({.text = "Grouped by the week they were taken.", .kind = widgets::TooltipKind::rich, .title = "Photos",
                        .action_label = "Learn more", .persistent = true},
                       fonts());
    t.anchor_to(anchor);
    t.set_window(window);
    t.handle({PointerAction::move, 110.0, 210.0});
    CHECK(t.is_open());
    t.handle({PointerAction::move, 5.0, 5.0});
    CHECK(t.is_open());
    step_for(t, 1.0);
    const render::Rect r = t.bounds();
    CHECK(r.width <= tokens::rich_tooltip_tokens.max_width);
    REQUIRE(t.action_bounds());
    const render::Rect a = *t.action_bounds();
    CHECK(a.y + a.height + tokens::rich_tooltip_tokens.action_bottom_padding <= r.y + r.height + 0.5);
    t.handle({PointerAction::move, a.x + 5.0, a.y + 5.0});
    t.handle({PointerAction::down, a.x + 5.0, a.y + 5.0});
    const widgets::TooltipResponse clicked = t.handle({PointerAction::up, a.x + 5.0, a.y + 5.0});
    CHECK(clicked.action);
    CHECK_FALSE(t.is_open());

    t.show();
    t.handle({PointerAction::down, 5.0, 5.0});
    CHECK_FALSE(t.is_open());
}

TEST_CASE("the caret points at the middle of the anchor from the tooltip's edge") {
    auto b = anchor_button(20.0, 200.0);
    widgets::Tooltip t = widgets::name_tooltip(b, fonts(), {.caret = true});
    t.set_window(window);
    t.show();
    const render::Rect r = t.bounds();
    REQUIRE(t.caret());
    const render::Rect c = *t.caret();
    CHECK(c.y == doctest::Approx(r.y + r.height));
    CHECK(c.height == tokens::tooltip_tokens.caret_height);
    CHECK(c.x + c.width / 2.0 == doctest::Approx(b.bounds().x + b.bounds().width / 2.0));
    CHECK(c.y + c.height + tokens::tooltip_tokens.anchor_spacing == doctest::Approx(b.bounds().y));
}

TEST_CASE("a tooltip paints in the inverse colors and reports itself while open") {
    auto b = anchor_button();
    widgets::Tooltip t = widgets::name_tooltip(b, fonts());
    t.set_window(window);
    CHECK_FALSE(t.accessible());
    t.show();
    step_for(t, 1.0);
    thorvg::Canvas canvas(600, 400);
    canvas.clear(0xffffffff);
    t.paint(canvas, scheme());
    const render::Rect r = t.bounds();
    CHECK(canvas.pixel(static_cast<int>(r.x + 3.0), static_cast<int>(r.y + r.height / 2.0)) ==
          scheme()[tokens::plain_tooltip_tokens.container]);
    const auto node = t.accessible();
    REQUIRE(node);
    CHECK(node->role == widgets::AccessibleRole::tooltip);
    CHECK(node->name == "Search");
}
