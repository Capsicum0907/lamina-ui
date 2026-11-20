#include <doctest/doctest.h>

#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/range_slider.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::RangeSlider;
using widgets::RangeThumb;

namespace {

void place(RangeSlider& s) {
    s.arrange({0.0, 0.0, 204.0, 44.0});
}

void press(RangeSlider& s, double x) {
    s.handle({PointerAction::down, x, 22.0});
    s.handle({PointerAction::up, x, 22.0});
}

}

static_assert(widgets::Measurable<RangeSlider>);
static_assert(widgets::Focusable<widgets::RangeSliderThumb>);
static_assert(widgets::FocusGated<widgets::RangeSliderThumb>);
static_assert(widgets::Disableable<RangeSlider>);

TEST_CASE("a range slider keeps its two values in order and inside the range") {
    RangeSlider s({.start = 0.8, .end = 0.2});
    CHECK(s.start() == doctest::Approx(0.2));
    CHECK(s.end() == doctest::Approx(0.8));
    s.set_range(-1.0, 3.0);
    CHECK(s.start() == 0.0);
    CHECK(s.end() == 1.0);
}

TEST_CASE("a press moves the nearer thumb, and thumbs meet but never cross") {
    RangeSlider s({.start = 0.25, .end = 0.75});
    place(s);
    press(s, 40.0);
    CHECK(s.start() == doctest::Approx(0.19));
    CHECK(s.end() == doctest::Approx(0.75));
    press(s, 180.0);
    CHECK(s.end() == doctest::Approx(0.89));
    s.handle({PointerAction::down, 40.0, 22.0});
    CHECK(s.dragging() == RangeThumb::start);
    s.handle({PointerAction::move, 200.0, 22.0});
    CHECK(s.start() == doctest::Approx(s.end()));
    s.handle({PointerAction::up, 200.0, 22.0});
    CHECK(s.start() == doctest::Approx(0.89));
}

TEST_CASE("when the thumbs coincide, the drag direction chooses which one moves") {
    RangeSlider left({.start = 0.5, .end = 0.5});
    place(left);
    left.handle({PointerAction::down, 102.0, 22.0});
    left.handle({PointerAction::move, 60.0, 22.0});
    CHECK(left.dragging() == RangeThumb::start);
    CHECK(left.start() < 0.5);
    CHECK(left.end() == doctest::Approx(0.5));
    left.handle({PointerAction::up, 60.0, 22.0});
    RangeSlider right({.start = 0.5, .end = 0.5});
    place(right);
    right.handle({PointerAction::down, 102.0, 22.0});
    right.handle({PointerAction::move, 150.0, 22.0});
    CHECK(right.dragging() == RangeThumb::end);
    CHECK(right.end() > 0.5);
}

TEST_CASE("steps snap both thumbs to the same ticks") {
    RangeSlider s({.start = 0.12, .end = 0.61, .steps = 3});
    CHECK(s.start() == doctest::Approx(0.0));
    CHECK(s.end() == doctest::Approx(0.5));
    place(s);
    CHECK(s.tick_centers().size() == 5);
}

TEST_CASE("each thumb is its own Tab stop with the slider keys") {
    RangeSlider s({.start = 0.2, .end = 0.6});
    place(s);
    widgets::FocusManager focus;
    focus.add(s.thumb(RangeThumb::start));
    focus.add(s.thumb(RangeThumb::end));
    focus.handle({Key::tab});
    CHECK(s.focused_thumb() == RangeThumb::start);
    CHECK(s.handle(input::KeyEvent{Key::right}));
    CHECK(s.start() == doctest::Approx(0.21));
    s.handle(input::KeyEvent{Key::end});
    CHECK(s.start() == doctest::Approx(0.6));
    CHECK_FALSE(s.handle(input::KeyEvent{Key::right}));
    focus.handle({Key::tab});
    CHECK(s.focused_thumb() == RangeThumb::end);
    s.handle(input::KeyEvent{Key::home});
    CHECK(s.end() == doctest::Approx(0.6));
    s.handle(input::KeyEvent{Key::page_up});
    CHECK(s.end() == doctest::Approx(0.7));
    s.handle(input::KeyEvent{Key::end});
    CHECK(s.end() == 1.0);
    CHECK(s.handle_width(RangeThumb::end) == 2.0);
    CHECK(s.handle_width(RangeThumb::start) == 4.0);
}

TEST_CASE("the track has an active part between the thumbs and stops at both ends") {
    RangeSlider s({.start = 0.25, .end = 0.75});
    place(s);
    const auto segs = s.segments();
    REQUIRE(segs.size() == 3);
    CHECK_FALSE(segs[0].active);
    CHECK(segs[0].to == doctest::Approx(50.0 - 8.0));
    CHECK(segs[1].active);
    CHECK(segs[1].from == doctest::Approx(58.0));
    CHECK(segs[1].to == doctest::Approx(142.0));
    CHECK(segs[2].from == doctest::Approx(158.0));
    s.set_range(0.5, 0.52);
    for (const auto& seg : s.segments()) {
        CHECK_FALSE(seg.active);
    }
}

TEST_CASE("each thumb reports a slider named for its end, limited by the other thumb") {
    RangeSlider s({.start = 0.2, .end = 0.6, .accessible_name = "Price"});
    place(s);
    const widgets::AccessibleNode n = s.accessible();
    CHECK(n.role == widgets::AccessibleRole::group);
    REQUIRE(n.children.size() == 2);
    CHECK(n.children[0].name == "Range start");
    CHECK(n.children[0].state.range->max == doctest::Approx(0.6));
    CHECK(n.children[1].name == "Range end");
    CHECK(n.children[1].state.range->min == doctest::Approx(0.2));
}

TEST_CASE("a disabled range slider takes no input and paints the disabled colors") {
    RangeSlider s({.start = 0.2, .end = 0.6, .enabled = false});
    place(s);
    press(s, 150.0);
    CHECK(s.end() == doctest::Approx(0.6));
    CHECK_FALSE(s.thumb(RangeThumb::start).can_focus());
    s.set_enabled(true);
    settle(s);
    thorvg::Canvas canvas(206, 44);
    canvas.clear(scheme()[color::Role::surface]);
    s.paint(canvas, scheme());
    canvas.finish();
    CHECK(canvas.pixel(100, 22) == scheme()[color::Role::primary]);
}
