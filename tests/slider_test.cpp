#include <doctest/doctest.h>

#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/widgets/slider.hpp"

using namespace lamina;
using platform::Key;
using platform::PointerAction;

namespace {

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

void settle(widgets::Slider& s) {
    for (int i = 0; i < 240; ++i) {
        s.step(1.0 / 60.0);
    }
}

widgets::Slider make(widgets::SliderOptions options) {
    widgets::Slider s(options);
    s.set_width(204.0);
    s.set_origin({0.0, 0.0});
    return s;
}

void click(widgets::Slider& s, double x) {
    s.handle({PointerAction::down, x, 22.0});
    s.handle({PointerAction::up, x, 22.0});
}

}

TEST_CASE("slider tokens follow SliderTokens.kt and Slider.kt") {
    const auto& t = tokens::slider_tokens;
    CHECK(t.track_height == 16.0);
    CHECK(t.handle_width == 4.0);
    CHECK(t.handle_height == 44.0);
    CHECK(t.pressed_handle_width == 2.0);
    CHECK(t.handle_track_gap == 6.0);
    CHECK(t.track_inside_corner == 2.0);
    CHECK(t.stop_indicator_size == 4.0);
    CHECK(t.active_track == color::Role::primary);
    CHECK(t.inactive_track == color::Role::secondary_container);
    CHECK(t.active_tick == color::Role::secondary_container);
    CHECK(t.inactive_tick == color::Role::primary);
}

TEST_CASE("a click sets the slider value from the pointer position") {
    auto s = make({});
    click(s, 102.0);
    CHECK(s.value() == doctest::Approx(0.5));
    click(s, 52.0);
    CHECK(s.value() == doctest::Approx(0.25));
    click(s, 1.0);
    CHECK(s.value() == 0.0);
    CHECK(s.handle_center_x() == doctest::Approx(2.0));
}

TEST_CASE("dragging keeps tracking outside the touch target and clamps at the ends") {
    auto s = make({0.2, 0.0, 10.0});
    CHECK(s.value() == doctest::Approx(0.2));
    CHECK(s.handle({PointerAction::down, 52.0, 22.0}));
    CHECK(s.dragging());
    s.handle({PointerAction::move, 152.0, 200.0});
    CHECK(s.value() == doctest::Approx(7.5));
    s.handle({PointerAction::move, 400.0, 200.0});
    CHECK(s.value() == doctest::Approx(10.0));
    s.handle({PointerAction::up, 400.0, 200.0});
    CHECK_FALSE(s.dragging());
    s.handle({PointerAction::move, 10.0, 22.0});
    CHECK(s.value() == doctest::Approx(10.0));
    CHECK_FALSE(s.handle({PointerAction::down, 100.0, 100.0}));
}

TEST_CASE("a stepped slider snaps to its ticks and draws the handle on them") {
    auto s = make({0.0, 0.0, 1.0, 4});
    const auto ticks = s.tick_centers();
    REQUIRE(ticks.size() == 6);
    CHECK(ticks.front() == doctest::Approx(10.0));
    CHECK(ticks.back() == doctest::Approx(194.0));
    click(s, ticks[2]);
    CHECK(s.value() == doctest::Approx(0.4));
    CHECK(s.handle_center_x() == doctest::Approx(ticks[2]));
    click(s, 0.0);
    CHECK(s.value() == 0.0);
    CHECK(s.handle_center_x() == doctest::Approx(2.0));
    click(s, 203.0);
    CHECK(s.value() == 1.0);
    CHECK(s.handle_center_x() == doctest::Approx(202.0));
    s.set_value(0.33);
    CHECK(s.value() == doctest::Approx(0.4));
}

TEST_CASE("the handle narrows at once while dragged") {
    auto s = make({});
    CHECK(s.handle_width() == 4.0);
    s.handle({PointerAction::down, 100.0, 22.0});
    CHECK(s.handle_width() == doctest::Approx(2.0));
    s.handle({PointerAction::up, 100.0, 22.0});
    CHECK(s.handle_width() == doctest::Approx(4.0));
}

TEST_CASE("arrow keys move by a step, or by a hundredth when continuous") {
    auto continuous = make({0.5});
    CHECK(continuous.handle(platform::KeyEvent{Key::right, false}));
    CHECK(continuous.value() == doctest::Approx(0.51));
    continuous.handle(platform::KeyEvent{Key::home, false});
    CHECK(continuous.value() == 0.0);
    CHECK_FALSE(continuous.handle(platform::KeyEvent{Key::left, false}));
    auto stepped = make({0.0, 0.0, 1.0, 4});
    stepped.handle(platform::KeyEvent{Key::right, false});
    CHECK(stepped.value() == doctest::Approx(0.2));
    stepped.handle(platform::KeyEvent{Key::end, false});
    CHECK(stepped.value() == 1.0);
}

TEST_CASE("the slider paints the active track, the gap, the handle and the inactive track") {
    auto s = make({0.5});
    render::ThorVGCanvas canvas(204, 44);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(canvas.pixel(20, 22) == scheme()[color::Role::primary]);
    CHECK(canvas.pixel(97, 22) == 0xffffffff);
    CHECK(canvas.pixel(102, 4) == scheme()[color::Role::primary]);
    CHECK(canvas.pixel(107, 22) == 0xffffffff);
    CHECK(canvas.pixel(160, 22) == scheme()[color::Role::secondary_container]);
    CHECK(canvas.pixel(194, 22) == scheme()[color::Role::primary]);
}
