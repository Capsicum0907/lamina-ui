#include <doctest/doctest.h>

#include <cmath>
#include <numbers>

#include "lamina/motion/spline_decay.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/touch_scroll.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using input::PointerKind;
using widgets::ScrollArea;

namespace {

constexpr const tokens::TouchScrollTokens& ts = tokens::touch_scroll;
constexpr double move_seconds = 0.008;

struct Child {
    render::Rect rect{20.0, 40.0, 100.0, 40.0};
    widgets::PressState press;
    int clicks = 0;
    int downs = 0;
    bool grabs = false;
    bool grabbed = false;

    bool handle(const input::PointerEvent& e) {
        if (e.action == PointerAction::down && widgets::contains(rect, e.x_dp, e.y_dp)) {
            ++downs;
            grabbed = grabs;
        }
        if (e.action == PointerAction::up || e.action == PointerAction::leave) {
            grabbed = false;
        }
        if (press.update(e, rect)) {
            ++clicks;
        }
        return grabbed;
    }
};

struct Finger {
    PointerKind kind = PointerKind::touch;
    double time = 1.0;

    input::PointerEvent at(PointerAction a, double x, double y) {
        input::PointerEvent e{a, x, y, a == PointerAction::down ? 1 : 0, kind, time};
        time += move_seconds;
        return e;
    }
};

ScrollArea vertical(double content = 2000.0) {
    ScrollArea s;
    s.set_viewport({0.0, 0.0, 300.0, 400.0});
    s.set_content_height(content);
    return s;
}

ScrollArea horizontal() {
    ScrollArea s;
    s.set_viewport({0.0, 100.0, 300.0, 60.0});
    s.set_content_height(60.0);
    s.set_content_width(1200.0);
    return s;
}

template <typename Deliver>
void swipe(ScrollArea& s, Finger& f, Deliver deliver, double x0, double y0, double dx, double dy, int steps, bool lift = true) {
    s.route(f.at(PointerAction::down, x0, y0), deliver);
    for (int i = 1; i <= steps; ++i) {
        s.route(f.at(PointerAction::move, x0 + dx * i / steps, y0 + dy * i / steps), deliver);
    }
    if (lift) {
        s.route(f.at(PointerAction::up, x0 + dx, y0 + dy), deliver);
    }
}

}

TEST_CASE("touch scroll tokens") {
    CHECK(ts.slop == 8.0);
    CHECK(ts.maximum_velocity == 8000.0);
    CHECK(ts.minimum_fling_velocity == 1.0);
    CHECK(ts.edge_tolerance == 0.5);
    CHECK(ts.horizontal_angle_limit_degrees == 30.0);
    CHECK(ts.vertical_angle_limit_degrees == 90.0);
    CHECK(tokens::fling_decay.friction == 0.015);
    CHECK(tokens::fling_decay.inflection == 0.35);
    CHECK(tokens::fling_decay.samples == 100);
}

TEST_CASE("spline decay follows the formulas of Compose's FlingCalculator") {
    const motion::SplineDecay d(tokens::fling_decay);
    const auto& s = tokens::fling_decay;
    const double coefficient = s.friction * s.gravity * s.inches_per_meter * s.dp_per_inch * s.physical_friction;
    const double rate = std::log(0.78) / std::log(0.9);
    const double v = 2000.0;
    const double l = std::log(s.inflection * v / coefficient);
    CHECK(d.duration_seconds(v) == doctest::Approx(std::exp(l / (rate - 1.0))));
    CHECK(d.distance(v) == doctest::Approx(coefficient * std::exp(rate / (rate - 1.0) * l)));
    CHECK(d.distance(-v) == doctest::Approx(-d.distance(v)));
    CHECK(std::abs(d.position(v, 0.0)) < 0.05);
    CHECK(d.position(v, d.duration_seconds(v)) == doctest::Approx(d.distance(v)));
    CHECK(d.velocity(v, d.duration_seconds(v) * 2.0) == 0.0);
    CHECK(d.distance(0.0) == 0.0);
    CHECK(d.duration_seconds(0.0) == 0.0);
}

TEST_CASE("spline decay starts at the fling velocity and slows down") {
    const motion::SplineDecay d(tokens::fling_decay);
    const double v = 3000.0;
    CHECK(d.velocity(v, 0.0) == doctest::Approx(v).epsilon(0.05));
    double last = 0.0;
    double previous_speed = d.velocity(v, 0.0);
    const double end = d.duration_seconds(v);
    for (double t = frame_seconds; t < end; t += frame_seconds) {
        const double p = d.position(v, t);
        CHECK(p >= last);
        last = p;
        const double speed = d.velocity(v, t);
        CHECK(speed <= previous_speed + 1e-9);
        previous_speed = speed;
    }
    CHECK(d.distance(2.0 * v) > d.distance(v));
    CHECK(d.duration_seconds(2.0 * v) > d.duration_seconds(v));
}

TEST_CASE("spline decay in dp at density 1 equals the pixel result divided by the density") {
    const motion::SplineDecay dp(tokens::fling_decay);
    for (double density : {2.625, 3.5}) {
        const motion::SplineDecay px(tokens::fling_decay, density);
        const double v = 1500.0;
        CHECK(px.distance(v * density) / density == doctest::Approx(dp.distance(v)));
        CHECK(px.duration_seconds(v * density) == doctest::Approx(dp.duration_seconds(v)));
        CHECK(px.position(v * density, 0.2) / density == doctest::Approx(dp.position(v, 0.2)));
    }
}

TEST_CASE("a touch inside the slop leaves the press to the child and a tap clicks") {
    auto s = vertical();
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    CHECK_FALSE(s.route(f.at(PointerAction::down, 50.0, 60.0), deliver));
    CHECK(child.press.pressed);
    CHECK_FALSE(s.route(f.at(PointerAction::move, 50.0, 60.0 - (ts.slop - 1.0)), deliver));
    CHECK(child.press.pressed);
    CHECK(s.offset() == 0.0);
    CHECK_FALSE(s.route(f.at(PointerAction::up, 50.0, 60.0 - (ts.slop - 1.0)), deliver));
    CHECK(child.clicks == 1);
}

TEST_CASE("past the slop the scroller takes the drag, follows the finger and cancels the child's press") {
    auto s = vertical();
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    s.route(f.at(PointerAction::down, 50.0, 60.0), deliver);
    CHECK(s.route(f.at(PointerAction::move, 50.0, 60.0 - (ts.slop + 2.0)), deliver));
    CHECK(s.offset() == doctest::Approx(2.0));
    CHECK(s.dragging());
    CHECK_FALSE(child.press.pressed);
    CHECK_FALSE(child.press.hovered);
    CHECK(s.route(f.at(PointerAction::move, 50.0, 60.0 - (ts.slop + 32.0)), deliver));
    CHECK(s.offset() == doctest::Approx(32.0));
    CHECK(s.route(f.at(PointerAction::up, 50.0, 60.0 - (ts.slop + 32.0)), deliver));
    CHECK(child.clicks == 0);
    CHECK_FALSE(child.press.pressed);
    CHECK_FALSE(s.dragging());
}

TEST_CASE("a mouse drag does not scroll") {
    auto s = vertical();
    Child child;
    Finger f{PointerKind::mouse};
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    swipe(s, f, deliver, 50.0, 60.0, 0.0, -200.0, 10);
    settle(s);
    CHECK(s.offset() == 0.0);
}

TEST_CASE("a fling continues after the finger lifts and decays to rest") {
    auto s = vertical(20000.0);
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    swipe(s, f, deliver, 150.0, 300.0, 0.0, -160.0, 8);
    const double lifted = s.offset();
    CHECK(s.flinging());
    CHECK(s.scroll_in_progress());
    s.step(frame_seconds);
    CHECK(s.offset() > lifted);
    int frames = 0;
    while (s.flinging() && frames < 10000) {
        s.step(frame_seconds);
        ++frames;
    }
    CHECK_FALSE(s.flinging());
    CHECK(s.offset() > lifted + 100.0);
    CHECK(child.clicks == 0);
}

TEST_CASE("a release after the finger held still does not fling") {
    auto s = vertical(20000.0);
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    swipe(s, f, deliver, 150.0, 300.0, 0.0, -160.0, 8, false);
    f.time += 0.2;
    s.route(f.at(PointerAction::up, 150.0, 140.0), deliver);
    CHECK_FALSE(s.flinging());
}

TEST_CASE("a fling stops at the edge") {
    auto s = vertical(800.0);
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    swipe(s, f, deliver, 150.0, 300.0, 0.0, -200.0, 4);
    CHECK(s.flinging());
    for (int i = 0; i < settle_frames && s.flinging(); ++i) {
        s.step(frame_seconds);
    }
    CHECK_FALSE(s.flinging());
    CHECK(s.offset() == doctest::Approx(s.max_offset()));
}

TEST_CASE("a touch during a fling stops it and is not a tap") {
    auto s = vertical(20000.0);
    Child child;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return child.handle(e); };
    swipe(s, f, deliver, 150.0, 300.0, 0.0, -160.0, 8);
    s.step(frame_seconds);
    REQUIRE(s.flinging());
    const double before = s.offset();
    CHECK(s.route(f.at(PointerAction::down, 50.0, 60.0), deliver));
    CHECK_FALSE(s.flinging());
    CHECK(child.downs == 0);
    CHECK(s.route(f.at(PointerAction::up, 50.0, 60.0), deliver));
    CHECK(child.clicks == 0);
    s.step(frame_seconds);
    CHECK(s.offset() == doctest::Approx(before));
}

TEST_CASE("a child that holds the press keeps the drag") {
    auto s = vertical();
    Child slider;
    slider.grabs = true;
    Finger f;
    auto deliver = [&](const input::PointerEvent& e) { return slider.handle(e); };
    swipe(s, f, deliver, 50.0, 60.0, 0.0, -100.0, 10);
    CHECK(s.offset() == 0.0);
    CHECK_FALSE(s.flinging());
}

namespace {

struct Nested {
    ScrollArea outer = vertical();
    ScrollArea inner = horizontal();
    Child button;
    Finger f;

    void route(PointerAction a, double x, double y) {
        auto to_button = [&](const input::PointerEvent& e) { return button.handle(e); };
        outer.route(f.at(a, x, y), [&](const input::PointerEvent& e) {
            inner.route(e, to_button);
            return inner.dragging();
        });
    }
    void drag(double angle_degrees, double length) {
        const double a = angle_degrees * std::numbers::pi / 180.0;
        const double x0 = 150.0;
        const double y0 = 130.0;
        route(PointerAction::down, x0, y0);
        for (int i = 1; i <= 10; ++i) {
            route(PointerAction::move, x0 - std::cos(a) * length * i / 10.0, y0 - std::sin(a) * length * i / 10.0);
        }
    }
};

}

TEST_CASE("a horizontal drag inside a vertical scroller moves the inner one") {
    Nested n;
    n.button.rect = {0.0, 100.0, 300.0, 60.0};
    n.drag(0.0, 100.0);
    CHECK(n.inner.offset_x() > 80.0);
    CHECK(n.outer.offset() == 0.0);
    CHECK_FALSE(n.button.press.pressed);
}

TEST_CASE("a vertical drag on a horizontal scroller moves the outer one") {
    Nested n;
    n.drag(90.0, 100.0);
    CHECK(n.outer.offset() > 80.0);
    CHECK(n.inner.offset_x() == 0.0);
}

TEST_CASE("a drag at 40 degrees goes to the vertical scroller, one at 20 degrees to the horizontal one") {
    Nested steep;
    steep.drag(40.0, 100.0);
    CHECK(steep.outer.offset() > 0.0);
    CHECK(steep.inner.offset_x() == 0.0);
    Nested shallow;
    shallow.drag(20.0, 100.0);
    CHECK(shallow.inner.offset_x() > 0.0);
    CHECK(shallow.outer.offset() == 0.0);
}

TEST_CASE("a lone horizontal scroller takes a steep drag once its axis passes the slop") {
    auto s = horizontal();
    Finger f;
    auto none = [](const input::PointerEvent&) { return false; };
    s.route(f.at(PointerAction::down, 150.0, 130.0), none);
    CHECK_FALSE(s.route(f.at(PointerAction::move, 150.0 - (ts.slop - 1.0), 100.0), none));
    CHECK(s.route(f.at(PointerAction::move, 150.0 - (ts.slop + 4.0), 90.0), none));
    CHECK(s.offset_x() == doctest::Approx(4.0));
}

TEST_CASE("the touch angle test follows Compose's bounds") {
    CHECK(widgets::angle_aligned(ts, {true, false}, 30.0));
    CHECK_FALSE(widgets::angle_aligned(ts, {true, false}, 31.0));
    CHECK(widgets::angle_aligned(ts, {false, true}, 31.0));
    CHECK_FALSE(widgets::angle_aligned(ts, {false, true}, 30.0));
    CHECK(widgets::angle_aligned(ts, {true, true}, 45.0));
}
