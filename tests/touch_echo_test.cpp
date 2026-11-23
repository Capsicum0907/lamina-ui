#include <doctest/doctest.h>

#include "lamina/input/touch_echo.hpp"
#include "lamina/tokens/interaction.hpp"

using lamina::input::PointerAction;
using lamina::input::PointerEvent;
using lamina::input::PointerKind;
using lamina::input::ScreenOrigin;
using lamina::input::TouchEchoFilter;

namespace {

constexpr double pixel_dp = 1.0 / 1.5;
constexpr double tolerance = lamina::tokens::touch_echo_tolerance_pixels * pixel_dp;
constexpr ScreenOrigin window{240.0, 66.0};

PointerEvent touch(PointerAction action, double x, double y) {
    return {action, x, y, 0, PointerKind::touch};
}

PointerEvent mouse(PointerAction action, double x, double y) {
    return {action, x, y, 0, PointerKind::mouse};
}

void tap(TouchEchoFilter& filter, double x, double y) {
    filter.admit(touch(PointerAction::move, x, y), window, tolerance);
    filter.admit(touch(PointerAction::down, x, y), window, tolerance);
    filter.admit(touch(PointerAction::up, x, y), window, tolerance);
}

}

TEST_CASE("before any touch every mouse move passes") {
    TouchEchoFilter filter;
    CHECK(filter.admit(mouse(PointerAction::move, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 296.0, 182.0), window, tolerance));
}

TEST_CASE("touch events always pass") {
    TouchEchoFilter filter;
    CHECK(filter.admit(touch(PointerAction::move, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(touch(PointerAction::down, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(touch(PointerAction::move, 296.0, 150.0), window, tolerance));
    CHECK(filter.admit(touch(PointerAction::up, 296.0, 150.0), window, tolerance));
}

TEST_CASE("after a tap a mouse move to the cursor left at the touch point is an echo, however often it comes") {
    TouchEchoFilter filter;
    tap(filter, 296.002, 182.002);
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 296.0, 182.0), window, tolerance));
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 296.0, 182.0), window, tolerance));
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 296.002 + tolerance * 0.9, 182.002 - tolerance * 0.9), window, tolerance));
}

TEST_CASE("while a finger is down every mouse move is an echo, also one at an earlier point of the drag") {
    TouchEchoFilter filter;
    filter.admit(touch(PointerAction::down, 700.0, 600.0), window, tolerance);
    filter.admit(touch(PointerAction::move, 700.0, 577.0), window, tolerance);
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 700.0, 587.0), window, tolerance));
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 10.0, 10.0), window, tolerance));
    filter.admit(touch(PointerAction::up, 700.0, 450.0), window, tolerance);
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 700.0, 450.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 700.0, 587.0), window, tolerance));
}

TEST_CASE("a mouse that moves away from the touch point passes at once") {
    TouchEchoFilter filter;
    tap(filter, 296.0, 182.0);
    CHECK(filter.admit(mouse(PointerAction::move, 296.0 + 2.0 * pixel_dp, 182.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 296.0, 182.0 - 2.0 * pixel_dp), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 500.0, 400.0), window, tolerance));
}

TEST_CASE("the touch point is kept on the screen, so an echo after the window moved is still an echo") {
    TouchEchoFilter filter;
    tap(filter, 296.0, 182.0);
    const ScreenOrigin moved{window.x_dp + 20.0, window.y_dp + 5.0};
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 276.0, 177.0), moved, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 296.0, 182.0), moved, tolerance));
}

TEST_CASE("presses, releases and leaves of the mouse are never held back, and a mouse press ends a finger that never lifted") {
    TouchEchoFilter filter;
    filter.admit(touch(PointerAction::down, 296.0, 182.0), window, tolerance);
    CHECK(filter.admit({PointerAction::leave, 0.0, 0.0}, window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::down, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::up, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 400.0, 300.0), window, tolerance));
}

TEST_CASE("pen moves keep their hover, also at the touch point") {
    TouchEchoFilter filter;
    tap(filter, 296.0, 182.0);
    CHECK(filter.admit({PointerAction::move, 296.0, 182.0, 0, PointerKind::pen}, window, tolerance));
    filter.admit(touch(PointerAction::down, 296.0, 182.0), window, tolerance);
    CHECK(filter.admit({PointerAction::move, 100.0, 100.0, 0, PointerKind::pen}, window, tolerance));
}

TEST_CASE("a touch that leaves without an up, as a cancelled one does, ends the finger") {
    TouchEchoFilter filter;
    filter.admit(touch(PointerAction::down, 296.0, 182.0), window, tolerance);
    CHECK_FALSE(filter.admit(mouse(PointerAction::move, 500.0, 400.0), window, tolerance));
    CHECK(filter.admit(touch(PointerAction::leave, 296.0, 182.0), window, tolerance));
    CHECK(filter.admit(mouse(PointerAction::move, 500.0, 400.0), window, tolerance));
}
