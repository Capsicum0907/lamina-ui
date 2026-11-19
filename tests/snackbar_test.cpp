#include <doctest/doctest.h>

#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/snackbar.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using widgets::Snackbar;

static_assert(widgets::Overlay<Snackbar> && widgets::Paintable<Snackbar> && widgets::Steppable<Snackbar>);

namespace {

constexpr const tokens::SnackbarTokens& t = tokens::snackbar_tokens;

void open(Snackbar& s, const char* message = "Task deleted") {
    s.set_area({0.0, 0.0, 400.0, 700.0});
    s.show(message);
    for (int i = 0; i < 30; ++i) {
        s.step(frame_seconds);
    }
}

render::Point center(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

}

TEST_CASE("snackbar tokens follow SnackbarTokens.kt, Snackbar.kt and SnackbarHost.kt") {
    CHECK(t.container == color::Role::inverse_surface);
    CHECK(t.corner == tokens::CornerSize::extra_small);
    CHECK(t.elevation == 3);
    CHECK(t.supporting_text_color == color::Role::inverse_on_surface);
    CHECK(t.supporting_text == tokens::TypeRole::body_medium);
    CHECK(t.action_color == color::Role::inverse_primary);
    CHECK(t.single_line_height == 48.0);
    CHECK(t.two_lines_height == 68.0);
    CHECK(t.max_width == 600.0);
    CHECK(t.outer_padding == 12.0);
    CHECK(t.start_padding == 16.0);
    CHECK(t.end_padding == 8.0);
    CHECK(t.text_vertical_padding == 14.0);
    CHECK(t.short_duration_seconds == 4.0);
    CHECK(t.long_duration_seconds == 10.0);
    CHECK(t.closed_scale == 0.8);
}

TEST_CASE("a snackbar sits at the bottom center and fills the width up to its maximum") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    CHECK(s.bounds().width == doctest::Approx(400.0 - 2.0 * t.outer_padding));
    CHECK(s.bounds().height == doctest::Approx(t.single_line_height));
    CHECK(s.bounds().y + s.bounds().height == doctest::Approx(700.0 - t.outer_padding));
    s.set_area({0.0, 0.0, 1400.0, 700.0});
    CHECK(s.bounds().width == doctest::Approx(t.max_width));
    CHECK(center(s.bounds()).x == doctest::Approx(700.0));
    const render::Rect a = *s.action_bounds();
    CHECK(a.x + a.width == doctest::Approx(s.bounds().x + s.bounds().width - t.end_padding));
    CHECK(center(a).y == doctest::Approx(center(s.bounds()).y));
}

TEST_CASE("a message too long for one line makes the snackbar taller") {
    Snackbar s({"Undo"}, fonts());
    open(s, "This message is long enough to need a second line");
    CHECK(s.bounds().height == doctest::Approx(t.two_lines_height));
}

TEST_CASE("show fades and scales the snackbar in, and it hides itself after its duration") {
    Snackbar s({"Undo"}, fonts());
    s.set_area({0.0, 0.0, 400.0, 700.0});
    CHECK_FALSE(s.visible());
    s.show("Task deleted");
    CHECK(s.is_open());
    CHECK(s.opacity() == 0.0);
    CHECK(s.scale() == doctest::Approx(t.closed_scale));
    s.step(frame_seconds);
    CHECK(s.opacity() > 0.0);
    for (int i = 0; i < 30; ++i) {
        s.step(frame_seconds);
    }
    CHECK_FALSE(s.animating());
    const double wake = s.next_wake_seconds();
    CHECK(wake == doctest::Approx(t.short_duration_seconds - 31 * frame_seconds));
    s.step(wake);
    CHECK_FALSE(s.is_open());
    CHECK(s.next_wake_seconds() < 0.0);
    settle(s);
    CHECK_FALSE(s.visible());
}

TEST_CASE("the pointer over the snackbar holds it open") {
    Snackbar s({"Undo", t.long_duration_seconds}, fonts());
    open(s);
    const render::Point c = center(s.bounds());
    CHECK(s.handle(input::PointerEvent{PointerAction::move, c.x, c.y}).consumed);
    CHECK(s.next_wake_seconds() < 0.0);
    s.step(2.0 * t.long_duration_seconds);
    CHECK(s.is_open());
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::move, 5.0, 5.0}).consumed);
    s.step(t.long_duration_seconds);
    CHECK_FALSE(s.is_open());
}

TEST_CASE("the action reports itself and closes the snackbar") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    const render::Point a = center(*s.action_bounds());
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::down, a.x, a.y}).action);
    const widgets::SnackbarResponse r = s.handle(input::PointerEvent{PointerAction::up, a.x, a.y});
    CHECK(r.action);
    CHECK(r.consumed);
    CHECK_FALSE(s.is_open());
    CHECK_FALSE(s.handle(input::PointerEvent{PointerAction::down, a.x, a.y}).consumed);
}

TEST_CASE("a snackbar without an action has no button") {
    Snackbar s({}, fonts());
    open(s);
    CHECK_FALSE(s.action_bounds());
}

TEST_CASE("a modeless snackbar on the overlay stack passes presses outside it to the base") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    widgets::OverlayStack overlays;
    int actions = 0;
    overlays.push(s, [&](const widgets::SnackbarResponse& r) { actions += r.action ? 1 : 0; }, widgets::Modality::modeless);
    CHECK_FALSE(overlays.handle(input::PointerEvent{PointerAction::down, 20.0, 20.0}));
    const render::Point a = center(*s.action_bounds());
    CHECK(overlays.handle(input::PointerEvent{PointerAction::down, a.x, a.y}));
    CHECK(overlays.handle(input::PointerEvent{PointerAction::up, a.x, a.y}));
    CHECK(actions == 1);
    CHECK(overlays.empty());
}

TEST_CASE("the snackbar paints its inverse surface") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    thorvg::Canvas canvas(400, 700);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    const render::Rect b = s.bounds();
    CHECK(canvas.pixel(static_cast<int>(b.x + 6.0), static_cast<int>(b.y + b.height / 2.0)) == scheme()[t.container]);
    CHECK(count_color(canvas, scheme()[t.action_color]) > 0);
}

static_assert(lamina::widgets::FocusGated<Snackbar>);

TEST_CASE("a snackbar's action takes the keyboard focus only while the snackbar is shown") {
    Snackbar s({"Undo"}, fonts());
    CHECK_FALSE(s.can_focus());
    open(s);
    CHECK(s.can_focus());
    s.set_focused(true);
    CHECK(s.focused());
    Snackbar plain({}, fonts());
    open(plain);
    CHECK_FALSE(plain.can_focus());
}

TEST_CASE("the time does not run while the action has the focus, and Space or Enter on it acts like a click") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    s.set_focused(true);
    CHECK(s.next_wake_seconds() < 0.0);
    for (int i = 0; i < 600; ++i) {
        s.step(1.0 / 60.0);
    }
    CHECK(s.is_open());
    CHECK_FALSE(s.handle(input::KeyEvent{input::Key::a, false}).action);
    const widgets::SnackbarResponse r = s.handle(input::KeyEvent{input::Key::enter, false});
    CHECK(r.action);
    CHECK(r.consumed);
    CHECK_FALSE(s.is_open());
    CHECK_FALSE(s.focused());
}

TEST_CASE("a snackbar ignores keys while its action has no focus, and dismissing drops the focus") {
    Snackbar s({"Undo"}, fonts());
    open(s);
    CHECK_FALSE(s.handle(input::KeyEvent{input::Key::space, false}).action);
    s.set_focused(true);
    s.dismiss();
    CHECK_FALSE(s.focused());
    s.set_focused(true);
    CHECK_FALSE(s.focused());
}
