#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/pull_to_refresh.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using input::PointerEvent;
using input::PointerKind;
using widgets::PullToRefresh;
using widgets::PullToRefreshResponse;

namespace {

constexpr const tokens::PullToRefreshTokens& t = tokens::pull_to_refresh_tokens;
constexpr render::Rect area{0.0, 100.0, 400.0, 600.0};
constexpr double x = 200.0;
constexpr double top = 150.0;

PointerEvent touch(PointerAction action, double px, double py) {
    return {action, px, py, 0, PointerKind::touch};
}

PullToRefresh make() {
    PullToRefresh p({.accessible_name = "Refresh"});
    p.set_area(area);
    return p;
}

PullToRefreshResponse drag_to(PullToRefresh& p, double from_y, double to_y, double step = 4.0) {
    PullToRefreshResponse last;
    double y = from_y;
    while (y < to_y) {
        y = std::min(to_y, y + step);
        const PullToRefreshResponse r = p.handle(touch(PointerAction::move, x, y), true);
        last.claimed = last.claimed || r.claimed;
        last.consumed = r.consumed;
    }
    return last;
}

double finger_for(double fraction) {
    return top + t.slop + widgets::pull_distance(fraction * t.threshold, t.threshold, t.max_tension) / t.drag_multiplier;
}

}

static_assert(widgets::Steppable<PullToRefresh>);
static_assert(widgets::Paintable<PullToRefresh>);

TEST_CASE("pull to refresh tokens: an 80 dp threshold, half the finger's travel, a 48 dp contained container") {
    CHECK(t.threshold == 80.0);
    CHECK(t.drag_multiplier == doctest::Approx(0.5));
    CHECK(t.max_tension == doctest::Approx(2.0));
    CHECK(t.container_size == tokens::loading_indicator_tokens.container_size);
    CHECK(t.container == color::Role::primary_container);
    CHECK(t.indicator == color::Role::on_primary_container);
    CHECK(t.slop == tokens::touch_slop);
}

TEST_CASE("the resistance follows the threshold, then gives way continuously and never passes twice the threshold") {
    double last = 0.0;
    for (double pulled = 0.0; pulled <= 4.0 * t.threshold; pulled += 0.5) {
        const double o = widgets::pull_offset(pulled, t.threshold, t.max_tension);
        CHECK(o >= last - 1e-9);
        CHECK(o - last <= 0.5 + 1e-9);
        CHECK(o <= 2.0 * t.threshold + 1e-9);
        last = o;
        if (pulled <= 3.0 * t.threshold) {
            CHECK(widgets::pull_distance(o, t.threshold, t.max_tension) == doctest::Approx(pulled));
        }
    }
    CHECK(widgets::pull_offset(t.threshold, t.threshold, t.max_tension) == doctest::Approx(t.threshold));
    CHECK(widgets::pull_offset(3.0 * t.threshold, t.threshold, t.max_tension) == doctest::Approx(2.0 * t.threshold));
}

TEST_CASE("a drag down from the top claims the touch past the slop and follows it from zero") {
    PullToRefresh p = make();
    CHECK_FALSE(p.handle(touch(PointerAction::down, x, top), true).consumed);
    CHECK_FALSE(p.handle(touch(PointerAction::move, x, top + t.slop), true).consumed);
    CHECK_FALSE(p.pulling());
    const PullToRefreshResponse r = p.handle(touch(PointerAction::move, x, top + t.slop + 1.0), true);
    CHECK(r.claimed);
    CHECK(r.consumed);
    CHECK(p.pulling());
    CHECK(p.holds_pointer());
    CHECK(p.distance_fraction() == doctest::Approx(t.drag_multiplier / t.threshold));
    double last = p.distance_fraction();
    for (double y = top + t.slop + 2.0; y < top + 600.0; y += 2.0) {
        p.handle(touch(PointerAction::move, x, y), true);
        CHECK(p.distance_fraction() >= last);
        CHECK(p.distance_fraction() - last <= 2.0 * t.drag_multiplier / t.threshold + 1e-9);
        last = p.distance_fraction();
    }
    CHECK(last == doctest::Approx(2.0));
}

TEST_CASE("released below the threshold, it springs back without a refresh") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(0.7));
    CHECK(p.distance_fraction() == doctest::Approx(0.7).epsilon(0.02));
    const PullToRefreshResponse r = p.handle(touch(PointerAction::up, x, finger_for(0.7)), true);
    CHECK(r.consumed);
    CHECK_FALSE(r.refresh);
    CHECK_FALSE(p.refreshing());
    CHECK(p.animating());
    double last = p.distance_fraction();
    for (int i = 0; i < settle_frames; ++i) {
        p.step(frame_seconds);
        CHECK(std::abs(p.distance_fraction() - last) < 0.2);
        last = p.distance_fraction();
    }
    CHECK(p.distance_fraction() == doctest::Approx(0.0).epsilon(1e-3));
    CHECK_FALSE(p.animating());
    CHECK_FALSE(p.shown());
}

TEST_CASE("released past the threshold, it asks for one refresh and rests at the threshold until told the refresh ended") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(1.4));
    CHECK(p.indicator().determinate());
    const PullToRefreshResponse r = p.handle(touch(PointerAction::up, x, finger_for(1.4)), true);
    CHECK(r.refresh);
    CHECK(p.refreshing());
    CHECK_FALSE(p.indicator().determinate());
    settle(p);
    CHECK(p.distance_fraction() == doctest::Approx(1.0).epsilon(1e-3));
    CHECK(p.container_rect().y == doctest::Approx(area.y + t.threshold - t.container_size).epsilon(1e-3));
    CHECK(p.animating());
    p.handle(touch(PointerAction::down, x, top), true);
    const PullToRefreshResponse again = drag_to(p, top, finger_for(1.5));
    CHECK_FALSE(again.claimed);
    CHECK_FALSE(p.handle(touch(PointerAction::up, x, finger_for(1.5)), true).refresh);
    p.set_refreshing(false);
    CHECK_FALSE(p.indicator().determinate());
    settle(p);
    CHECK_FALSE(p.shown());
    CHECK(p.indicator().determinate());
    CHECK_FALSE(p.animating());
}

TEST_CASE("pulling back up shrinks the pull to nothing and a release there does not refresh") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(1.2));
    CHECK(p.distance_fraction() > 1.0);
    double last = p.distance_fraction();
    for (double y = finger_for(1.2); y > top - 50.0; y -= 3.0) {
        p.handle(touch(PointerAction::move, x, y), true);
        CHECK(p.distance_fraction() <= last + 1e-9);
        last = p.distance_fraction();
    }
    CHECK(p.distance_fraction() == 0.0);
    CHECK(p.pulling());
    CHECK_FALSE(p.handle(touch(PointerAction::up, x, top - 50.0), true).refresh);
}

TEST_CASE("a list that is not at its top does not pull, and one that reaches it during the drag pulls from there") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), false);
    for (double y = top; y < top + 100.0; y += 4.0) {
        CHECK_FALSE(p.handle(touch(PointerAction::move, x, y), false).consumed);
    }
    CHECK_FALSE(p.pulling());
    const double reached = top + 100.0;
    CHECK_FALSE(p.handle(touch(PointerAction::move, x, reached), true).consumed);
    CHECK_FALSE(p.handle(touch(PointerAction::move, x, reached + t.slop), true).consumed);
    CHECK(p.handle(touch(PointerAction::move, x, reached + t.slop + 2.0), true).claimed);
    CHECK(p.distance_fraction() == doctest::Approx(2.0 * t.drag_multiplier / t.threshold));
}

TEST_CASE("a sideways drag, a drag up, a mouse and a press outside the area do not pull") {
    PullToRefresh side = make();
    side.handle(touch(PointerAction::down, x, top), true);
    for (double d = 0.0; d < 100.0; d += 4.0) {
        CHECK_FALSE(side.handle(touch(PointerAction::move, x + 2.0 * d, top + d), true).consumed);
    }
    PullToRefresh up = make();
    up.handle(touch(PointerAction::down, x, top), true);
    for (double y = top; y > top - 40.0; y -= 4.0) {
        CHECK_FALSE(up.handle(touch(PointerAction::move, x, y), true).consumed);
    }
    PullToRefresh mouse = make();
    mouse.handle({PointerAction::down, x, top}, true);
    for (double y = top; y < top + 200.0; y += 4.0) {
        CHECK_FALSE(mouse.handle({PointerAction::move, x, y}, true).consumed);
    }
    PullToRefresh outside = make();
    outside.handle(touch(PointerAction::down, x, area.y - 20.0), true);
    for (double y = area.y - 20.0; y < top + 200.0; y += 4.0) {
        CHECK_FALSE(outside.handle(touch(PointerAction::move, x, y), true).consumed);
    }
}

TEST_CASE("a cancelled pull, a lost touch and a disabled pull spring back without a refresh") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(1.5));
    p.cancel_interaction();
    CHECK_FALSE(p.pulling());
    CHECK_FALSE(p.handle(touch(PointerAction::up, x, finger_for(1.5)), true).refresh);
    settle(p);
    CHECK_FALSE(p.shown());

    PullToRefresh lost = make();
    lost.handle(touch(PointerAction::down, x, top), true);
    drag_to(lost, top, finger_for(1.5));
    const PullToRefreshResponse r = lost.handle({PointerAction::leave, 0.0, 0.0}, true);
    CHECK(r.consumed);
    CHECK_FALSE(r.refresh);
    CHECK_FALSE(lost.refreshing());

    PullToRefresh off = make();
    off.handle(touch(PointerAction::down, x, top), true);
    drag_to(off, top, finger_for(0.5));
    off.set_enabled(false);
    CHECK_FALSE(off.pulling());
    CHECK_FALSE(drag_to(off, finger_for(0.5), finger_for(1.5)).consumed);
}

TEST_CASE("a pull that starts while the indicator is still leaving takes it from where it is") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(0.9));
    p.handle(touch(PointerAction::up, x, finger_for(0.9)), true);
    for (int i = 0; i < 3; ++i) {
        p.step(frame_seconds);
    }
    const double leaving = p.distance_fraction();
    REQUIRE(leaving > 0.1);
    p.handle(touch(PointerAction::down, x, top), true);
    p.handle(touch(PointerAction::move, x, top + t.slop + 1.0), true);
    CHECK(p.pulling());
    CHECK(p.distance_fraction() == doctest::Approx(leaving).epsilon(1e-6));
}

TEST_CASE("a refresh started by the program brings the indicator in, and its end takes it out on the spring") {
    PullToRefresh p = make();
    CHECK_FALSE(p.shown());
    CHECK_FALSE(p.accessible());
    p.set_refreshing(true);
    CHECK(p.refreshing());
    CHECK(p.shown());
    CHECK_FALSE(p.indicator().determinate());
    double last = p.distance_fraction();
    for (int i = 0; i < settle_frames; ++i) {
        p.step(frame_seconds);
        CHECK(std::abs(p.distance_fraction() - last) < 0.2);
        last = p.distance_fraction();
    }
    CHECK(p.distance_fraction() == doctest::Approx(1.0).epsilon(1e-3));
    REQUIRE(p.accessible());
    CHECK(p.accessible()->role == widgets::AccessibleRole::progress_indicator);
    CHECK(p.accessible()->name == "Refresh");
    p.set_refreshing(false);
    for (int i = 0; i < settle_frames; ++i) {
        p.step(frame_seconds);
        CHECK(std::abs(p.distance_fraction() - last) < 0.2);
        last = p.distance_fraction();
    }
    CHECK_FALSE(p.shown());
}

TEST_CASE("the indicator grows and turns with the pull and keeps its turn when the refresh starts") {
    PullToRefresh p = make();
    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(0.25));
    CHECK(p.indicator_scale() == doctest::Approx(t.start_scale + (1.0 - t.start_scale) * 0.25).epsilon(0.02));
    drag_to(p, finger_for(0.25), finger_for(1.0));
    CHECK(p.indicator_scale() == doctest::Approx(1.0).epsilon(0.01));
    CHECK(p.indicator_turn_degrees() == doctest::Approx(0.0).epsilon(0.02));
    drag_to(p, finger_for(1.0), finger_for(1.5));
    const double turned = p.indicator_turn_degrees();
    CHECK(turned == doctest::Approx(0.5 * t.overshoot_turn_degrees).epsilon(0.02));
    const double shape_turn = p.indicator().rotation_degrees();
    p.handle(touch(PointerAction::up, x, finger_for(1.5)), true);
    CHECK(p.indicator_turn_degrees() == turned);
    CHECK(p.indicator().rotation_degrees() == doctest::Approx(shape_turn));
}

TEST_CASE("the container comes in from behind the top of the area and is cut there") {
    const color::ColorScheme& s = scheme();
    PullToRefresh p = make();
    thorvg::Canvas idle(400, 800);
    idle.clear(0xffffffff);
    p.paint(idle, s);
    CHECK(count_color(idle, s[t.container]) == 0);

    p.handle(touch(PointerAction::down, x, top), true);
    drag_to(p, top, finger_for(0.3));
    const render::Rect r = p.container_rect();
    CHECK(r.y < area.y);
    CHECK(r.y + r.height > area.y);
    thorvg::Canvas part(400, 800);
    part.clear(0xffffffff);
    p.paint(part, s);
    CHECK(part.pixel(200, static_cast<int>(area.y) - 2) == 0xffffffff);
    CHECK(part.pixel(200, static_cast<int>(area.y) + 1) != 0xffffffff);

    drag_to(p, finger_for(0.3), finger_for(1.0));
    thorvg::Canvas full(400, 800);
    full.clear(0xffffffff);
    p.paint(full, s);
    const render::Rect c = p.container_rect();
    CHECK(full.pixel(static_cast<int>(c.x + 3.0), static_cast<int>(c.y + c.height / 2.0)) == s[t.container]);
}
