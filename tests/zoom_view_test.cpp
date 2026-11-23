#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>

#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/zoom_view.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::FingerAction;
using input::FingerEvent;
using input::PointerAction;
using widgets::ZoomView;

static_assert(widgets::Steppable<ZoomView> && widgets::HandlesPointer<ZoomView> && widgets::Cancellable<ZoomView> &&
              widgets::HoldsPointer<ZoomView> && widgets::Accessible<ZoomView>);

namespace {

constexpr const tokens::ZoomTokens& t = tokens::zoom;
constexpr render::Rect area{100.0, 50.0, 400.0, 300.0};
constexpr double step_seconds = 1.0 / 120.0;

ZoomView make() {
    ZoomView z({.accessible_name = "Photo", .content_width = 800.0, .content_height = 400.0});
    z.set_bounds(area);
    return z;
}

struct Clock {
    double now = 1.0;
    double tick() { return now += step_seconds; }
};

FingerEvent finger(FingerAction a, std::uint64_t id, double x, double y, double time) {
    return {a, id, x, y, time};
}

void run(ZoomView& z) {
    for (int i = 0; i < settle_frames; ++i) {
        z.step(frame_seconds);
    }
}

double content_x(const ZoomView& z, double window_x) {
    return motion::content_x(z.geometry(), z.transform(), window_x - area.x);
}

double content_y(const ZoomView& z, double window_y) {
    return motion::content_y(z.geometry(), z.transform(), window_y - area.y);
}

void pinch(ZoomView& z, Clock& c, double cx, double cy, double from_half, double to_half, int steps) {
    z.handle(finger(FingerAction::down, 1, cx - from_half, cy, c.tick()));
    z.handle(finger(FingerAction::down, 2, cx + from_half, cy, c.tick()));
    for (int i = 1; i <= steps; ++i) {
        const double h = from_half + (to_half - from_half) * i / steps;
        const double time = c.tick();
        z.handle(finger(FingerAction::move, 1, cx - h, cy, time));
        z.handle(finger(FingerAction::move, 2, cx + h, cy, time));
    }
}

void lift(ZoomView& z, Clock& c, double cx, double cy, double half) {
    const double time = c.tick();
    z.handle(finger(FingerAction::up, 1, cx - half, cy, time));
    z.handle(finger(FingerAction::up, 2, cx + half, cy, time));
}

void tap(ZoomView& z, double x, double y, double down, double up) {
    z.handle(finger(FingerAction::down, 7, x, y, down));
    z.handle(finger(FingerAction::up, 7, x, y, up));
}

void drag(ZoomView& z, Clock& c, double x, double y, double dx, double dy, int steps) {
    z.handle(finger(FingerAction::down, 3, x, y, c.tick()));
    for (int i = 1; i <= steps; ++i) {
        z.handle(finger(FingerAction::move, 3, x + dx * i / steps, y + dy * i / steps, c.tick()));
    }
}

void zoom_in(ZoomView& z, double scale) {
    z.zoom_to(scale, area.x + area.width / 2.0, area.y + area.height / 2.0);
    run(z);
}

}

TEST_CASE("a zoom view starts fitted and centred") {
    const ZoomView z = make();
    CHECK(z.scale() == 1.0);
    CHECK(!z.zoomed());
    CHECK(z.transform().x == 0.0);
    CHECK(z.transform().y == doctest::Approx(50.0));
    CHECK(!z.animating());
    const auto node = z.accessible();
    REQUIRE(node);
    CHECK(node->role == widgets::AccessibleRole::group);
    CHECK(node->name == "Photo");
    REQUIRE(node->state.range);
    CHECK(node->state.range->value == 1.0);
    CHECK(node->state.range->max == t.max_scale);
}

TEST_CASE("pinching apart scales by the span about the focus") {
    ZoomView z = make();
    Clock c;
    const double cx = 300.0;
    const double cy = 200.0;
    const double before_x = content_x(z, cx);
    const double before_y = content_y(z, cy);
    pinch(z, c, cx, cy, 100.0, 200.0, 10);
    CHECK(z.holds_pointer());
    CHECK(z.scale() == doctest::Approx(400.0 / 220.0));
    CHECK(content_x(z, cx) == doctest::Approx(before_x));
    CHECK(content_y(z, cy) == doctest::Approx(before_y));
    lift(z, c, cx, cy, 200.0);
    run(z);
    CHECK(z.scale() == doctest::Approx(400.0 / 220.0));
    CHECK(motion::in_bounds(z.geometry(), z.transform(), {t.min_scale, t.max_scale}, 1e-6));
}

TEST_CASE("the focus moving during a pinch pans the content with it") {
    ZoomView z = make();
    Clock c;
    pinch(z, c, 300.0, 200.0, 100.0, 150.0, 5);
    const double cx = content_x(z, 300.0);
    const double time = c.tick();
    z.handle(finger(FingerAction::move, 1, 300.0 - 150.0 + 30.0, 200.0, time));
    z.handle(finger(FingerAction::move, 2, 300.0 + 150.0 + 30.0, 200.0, time));
    CHECK(content_x(z, 330.0) == doctest::Approx(cx));
}

TEST_CASE("fingers closer than the minimum span do not scale, and a small change does not start it") {
    ZoomView z = make();
    Clock c;
    pinch(z, c, 300.0, 200.0, 40.0, 80.0, 8);
    CHECK(z.scale() == 1.0);
    lift(z, c, 300.0, 200.0, 80.0);
    pinch(z, c, 300.0, 200.0, 100.0, 107.0, 7);
    CHECK(z.scale() == 1.0);
}

TEST_CASE("a pinch past the maximum resists and springs back to it") {
    ZoomView z = make();
    Clock c;
    zoom_in(z, 4.0);
    pinch(z, c, 300.0, 200.0, 90.0, 190.0, 20);
    CHECK(z.scale() > t.max_scale);
    CHECK(z.scale() < t.max_scale * t.scale_band_ratio);
    lift(z, c, 300.0, 200.0, 190.0);
    CHECK(z.animating());
    run(z);
    CHECK(z.scale() == doctest::Approx(t.max_scale));
    CHECK(motion::in_bounds(z.geometry(), z.transform(), {t.min_scale, t.max_scale}, 1e-6));
}

TEST_CASE("a pinch below the minimum springs back to the fitted view") {
    ZoomView z = make();
    Clock c;
    pinch(z, c, 300.0, 200.0, 190.0, 95.0, 10);
    CHECK(z.scale() < 1.0);
    lift(z, c, 300.0, 200.0, 95.0);
    run(z);
    CHECK(z.scale() == doctest::Approx(1.0));
    CHECK(z.transform().x == doctest::Approx(0.0));
    CHECK(z.transform().y == doctest::Approx(50.0));
}

TEST_CASE("one finger at the fitted scale leaves the drag to the page") {
    ZoomView z = make();
    Clock c;
    drag(z, c, 300.0, 200.0, 0.0, 120.0, 10);
    CHECK(!z.holds_pointer());
    CHECK(z.transform().y == doctest::Approx(50.0));
}

TEST_CASE("one finger pans a zoomed view after the slop and holds the pointer from the down") {
    ZoomView z = make();
    Clock c;
    zoom_in(z, 2.0);
    const motion::ZoomTransform start = z.transform();
    z.handle(finger(FingerAction::down, 3, 300.0, 200.0, c.tick()));
    CHECK(z.holds_pointer());
    z.handle(finger(FingerAction::move, 3, 300.0, 205.0, c.tick()));
    CHECK(z.transform().y == start.y);
    z.handle(finger(FingerAction::move, 3, 300.0, 230.0, c.tick()));
    CHECK(z.transform().y == doctest::Approx(start.y + 30.0 - t.touch_slop));
}

TEST_CASE("a pan past the edge resists and springs back") {
    ZoomView z = make();
    Clock c;
    zoom_in(z, 2.0);
    drag(z, c, 200.0, 200.0, 300.0, 0.0, 30);
    const motion::ZoomLimits lx = motion::pan_limits_x(z.geometry(), z.scale());
    CHECK(z.transform().x > lx.max);
    CHECK(z.transform().x < lx.max + 300.0 - t.touch_slop);
    c.tick();
    c.tick();
    c.tick();
    c.tick();
    c.tick();
    c.tick();
    z.handle(finger(FingerAction::up, 3, 500.0, 200.0, c.now + 0.1));
    run(z);
    CHECK(z.transform().x == doctest::Approx(lx.max));
    CHECK(z.edges().left);
}

TEST_CASE("a quick pan flings and stops at the edge") {
    ZoomView z = make();
    Clock c;
    zoom_in(z, 3.0);
    const double start_x = z.transform().x;
    drag(z, c, 400.0, 200.0, -60.0, 0.0, 6);
    const double released_x = z.transform().x;
    z.handle(finger(FingerAction::up, 3, 340.0, 200.0, c.tick()));
    CHECK(z.animating());
    z.step(0.1);
    CHECK(z.transform().x < released_x);
    run(z);
    CHECK(!z.animating());
    CHECK(z.transform().x < released_x);
    CHECK(z.transform().x >= motion::pan_limits_x(z.geometry(), z.scale()).min);
    CHECK(start_x > released_x);
}

TEST_CASE("a double tap zooms in about the first tap and a second one fits again") {
    ZoomView z = make();
    const double x = 200.0;
    const double y = 150.0;
    const double cx = content_x(z, x);
    const double cy = content_y(z, y);
    tap(z, x, y, 1.0, 1.05);
    CHECK(!z.animating());
    tap(z, x + 10.0, y, 1.2, 1.25);
    CHECK(z.animating());
    run(z);
    CHECK(z.scale() == doctest::Approx(t.double_tap_scale));
    CHECK(content_x(z, x) == doctest::Approx(cx));
    CHECK(content_y(z, y) == doctest::Approx(cy));
    tap(z, x, y, 2.0, 2.05);
    tap(z, x, y, 2.2, 2.25);
    run(z);
    CHECK(z.scale() == doctest::Approx(1.0));
    CHECK(z.transform().y == doctest::Approx(50.0));
}

TEST_CASE("taps too slow, too quick, too far apart or with a moving first tap are not a double tap") {
    ZoomView z = make();
    tap(z, 200.0, 150.0, 1.0, 1.05);
    tap(z, 200.0, 150.0, 1.05 + t.double_tap_timeout_seconds + 0.05, 1.5);
    run(z);
    CHECK(z.scale() == 1.0);
    tap(z, 200.0, 150.0, 3.0, 3.05);
    tap(z, 200.0, 150.0, 3.05 + t.double_tap_min_seconds / 2.0, 3.1);
    run(z);
    CHECK(z.scale() == 1.0);
    tap(z, 200.0, 150.0, 5.0, 5.05);
    tap(z, 200.0 + t.double_tap_slop, 150.0, 5.2, 5.25);
    run(z);
    CHECK(z.scale() == 1.0);
    z.handle(finger(FingerAction::down, 7, 200.0, 150.0, 7.0));
    z.handle(finger(FingerAction::move, 7, 200.0 + t.double_tap_touch_slop + 1.0, 150.0, 7.02));
    z.handle(finger(FingerAction::up, 7, 200.0 + t.double_tap_touch_slop + 1.0, 150.0, 7.05));
    tap(z, 200.0, 150.0, 7.2, 7.25);
    run(z);
    CHECK(z.scale() == 1.0);
}

TEST_CASE("lifting one finger of a pinch goes on panning with the other") {
    ZoomView z = make();
    Clock c;
    pinch(z, c, 300.0, 200.0, 100.0, 200.0, 10);
    const double scale = z.scale();
    z.handle(finger(FingerAction::up, 1, 100.0, 200.0, c.tick()));
    CHECK(z.holds_pointer());
    const double y = z.transform().y;
    z.handle(finger(FingerAction::move, 2, 500.0, 190.0, c.tick()));
    z.handle(finger(FingerAction::move, 2, 500.0, 170.0, c.tick()));
    CHECK(z.scale() == doctest::Approx(scale));
    CHECK(z.transform().y < y);
}

TEST_CASE("a leave while the touch is down hands the gesture to the scroller around") {
    ZoomView z = make();
    Clock c;
    z.handle(finger(FingerAction::down, 3, 300.0, 200.0, c.tick()));
    z.handle(input::PointerEvent{PointerAction::down, 300.0, 200.0, 1, input::PointerKind::touch});
    CHECK(!z.holds_pointer());
    z.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0});
    z.handle(finger(FingerAction::down, 4, 400.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::move, 3, 150.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::move, 4, 480.0, 200.0, c.tick()));
    CHECK(z.scale() == 1.0);
    CHECK(!z.holds_pointer());
    z.handle(finger(FingerAction::up, 3, 150.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::up, 4, 480.0, 200.0, c.tick()));
    pinch(z, c, 300.0, 200.0, 100.0, 200.0, 10);
    CHECK(z.scale() > 1.0);
}

TEST_CASE("the leave after a lifted finger does not end the pinch of the other") {
    ZoomView z = make();
    Clock c;
    zoom_in(z, 2.0);
    z.handle(finger(FingerAction::down, 1, 200.0, 200.0, c.tick()));
    z.handle(input::PointerEvent{PointerAction::down, 200.0, 200.0, 1, input::PointerKind::touch});
    z.handle(finger(FingerAction::down, 2, 420.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::up, 1, 200.0, 200.0, c.tick()));
    z.handle(input::PointerEvent{PointerAction::up, 200.0, 200.0, 0, input::PointerKind::touch});
    z.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0});
    CHECK(z.holds_pointer());
    const double x = z.transform().x;
    z.handle(finger(FingerAction::move, 2, 400.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::move, 2, 380.0, 200.0, c.tick()));
    CHECK(z.transform().x < x);
}

TEST_CASE("fingers that start outside the view are not followed") {
    ZoomView z = make();
    Clock c;
    z.handle(finger(FingerAction::down, 1, 50.0, 200.0, c.tick()));
    z.handle(finger(FingerAction::down, 2, 300.0, 200.0, c.tick()));
    CHECK(z.fingers_down() == 1);
    z.handle(finger(FingerAction::move, 2, 480.0, 200.0, c.tick()));
    CHECK(z.scale() == 1.0);
}

TEST_CASE("Ctrl and the wheel zoom about the cursor; the plain wheel is left to the page") {
    ZoomView z = make();
    const double x = 180.0;
    const double y = 120.0;
    const double cx = content_x(z, x);
    CHECK(!z.handle(input::WheelEvent{x, y, 0.0, -1.0}));
    CHECK(z.handle(input::WheelEvent{x, y, 0.0, -1.0, false, true}));
    CHECK(z.handle(input::WheelEvent{x, y, 0.0, -1.0, false, true}));
    run(z);
    CHECK(z.scale() == doctest::Approx(t.wheel_zoom_factor * t.wheel_zoom_factor));
    CHECK(content_x(z, x) == doctest::Approx(cx));
    for (int i = 0; i < 30; ++i) {
        z.handle(input::WheelEvent{x, y, 0.0, -1.0, false, true});
    }
    run(z);
    CHECK(z.scale() == doctest::Approx(t.max_scale));
    for (int i = 0; i < 30; ++i) {
        z.handle(input::WheelEvent{x, y, 0.0, 1.0, false, true});
    }
    run(z);
    CHECK(z.scale() == doctest::Approx(t.min_scale));
    CHECK(!z.handle(input::WheelEvent{10.0, 10.0, 0.0, -1.0, false, true}));
}

TEST_CASE("the mouse drags a zoomed view and a double click toggles the zoom") {
    ZoomView z = make();
    CHECK(!z.handle(input::PointerEvent{PointerAction::down, 300.0, 200.0, 1}));
    z.handle(input::PointerEvent{PointerAction::up, 300.0, 200.0});
    CHECK(z.handle(input::PointerEvent{PointerAction::down, 300.0, 200.0, 2}));
    z.handle(input::PointerEvent{PointerAction::up, 300.0, 200.0});
    run(z);
    CHECK(z.scale() == doctest::Approx(t.double_tap_scale));
    const double x = z.transform().x;
    CHECK(z.handle(input::PointerEvent{PointerAction::down, 300.0, 200.0, 1}));
    CHECK(z.holds_pointer());
    CHECK(z.cursor() == input::Cursor::move);
    z.handle(input::PointerEvent{PointerAction::move, 260.0, 200.0});
    CHECK(z.transform().x == doctest::Approx(x - 40.0));
    z.handle(input::PointerEvent{PointerAction::up, 260.0, 200.0});
    CHECK(!z.holds_pointer());
    CHECK(!z.cursor());
}

TEST_CASE("a pager can ask whether a horizontal drag would move the content") {
    ZoomView z = make();
    CHECK(z.at_min_scale());
    CHECK(!z.can_pan_horizontally(10.0));
    CHECK(!z.can_pan_horizontally(-10.0));
    z.zoom_to(2.0, area.x, area.y + area.height / 2.0);
    run(z);
    CHECK(!z.at_min_scale());
    CHECK(z.edges().left);
    CHECK(!z.can_pan_horizontally(10.0));
    CHECK(z.can_pan_horizontally(-10.0));
    CHECK(z.can_pan_vertically(10.0));
    CHECK(z.can_pan_vertically(-10.0));
}

TEST_CASE("a disabled zoom view ignores input and a cancel settles a gesture") {
    ZoomView z = make();
    Clock c;
    z.set_enabled(false);
    pinch(z, c, 300.0, 200.0, 100.0, 200.0, 10);
    CHECK(z.scale() == 1.0);
    CHECK(z.fingers_down() == 0);
    z.set_enabled(true);
    pinch(z, c, 300.0, 200.0, 190.0, 90.0, 10);
    CHECK(z.scale() < 1.0);
    z.cancel_interaction();
    CHECK(z.fingers_down() == 0);
    run(z);
    CHECK(z.scale() == doctest::Approx(1.0));
}

TEST_CASE("a new size keeps the scale and clamps the pan") {
    ZoomView z = make();
    zoom_in(z, 3.0);
    z.set_bounds({100.0, 50.0, 300.0, 300.0});
    CHECK(z.scale() == doctest::Approx(3.0));
    CHECK(motion::in_bounds(z.geometry(), z.transform(), {t.min_scale, t.max_scale}, 1e-6));
}
