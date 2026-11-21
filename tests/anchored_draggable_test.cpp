#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/tokens/motion.hpp"

using namespace lamina;

namespace {

enum class Stop { low, middle, high };

using Drag = motion::AnchoredDraggable<Stop>;

constexpr double fixed_threshold = 56.0;
constexpr double velocity_threshold = 125.0;
constexpr double frame = 1.0 / 60.0;

Drag make(Stop initial = Stop::low) {
    Drag d(initial, {[](double) { return fixed_threshold; }, velocity_threshold}, tokens::drag_gesture);
    d.set_anchors({{Stop::low, 0.0}, {Stop::middle, 300.0}, {Stop::high, 600.0}}, initial);
    return d;
}

void run(Drag& d, double seconds = 3.0) {
    for (double t = 0.0; t < seconds; t += frame) {
        d.step(frame);
    }
}

}

TEST_CASE("the velocity tracker fits a line through steady motion") {
    motion::VelocityTracker v(tokens::velocity_tracking);
    for (int i = 0; i <= 6; ++i) {
        v.add(i * 0.010, 100.0 + i * 5.0);
    }
    CHECK(v.velocity() == doctest::Approx(500.0).epsilon(0.001));
}

TEST_CASE("the velocity tracker ignores samples older than the horizon and after a pause") {
    motion::VelocityTracker v(tokens::velocity_tracking);
    v.add(0.0, 0.0);
    v.add(0.010, 10.0);
    v.add(0.200, 10.0);
    CHECK(v.velocity() == doctest::Approx(0.0));
    v.add(0.210, 20.0);
    CHECK(v.velocity() == doctest::Approx(1000.0).epsilon(0.001));
}

TEST_CASE("releasing after the pointer stopped for longer than 40 ms forgets the motion") {
    motion::VelocityTracker v(tokens::velocity_tracking);
    v.add(0.0, 0.0);
    v.add(0.010, 10.0);
    v.release(0.060);
    CHECK(v.velocity() == doctest::Approx(0.0));
}

TEST_CASE("the quadratic fit takes the slope at the newest sample") {
    std::vector<double> t{0.0, -10.0, -20.0, -30.0};
    std::vector<double> y;
    for (double x : t) {
        y.push_back(2.0 + 3.0 * x + 0.5 * x * x);
    }
    CHECK(motion::least_squares_slope(t, y, 2) == doctest::Approx(3.0));
}

TEST_CASE("a drag starts past the slop and follows the pointer from there") {
    Drag d = make();
    d.press(100.0, 0.0);
    CHECK_FALSE(d.move(100.0 + tokens::touch_slop, 0.01));
    CHECK(d.offset() == doctest::Approx(0.0));
    CHECK(d.move(150.0, 0.02));
    CHECK(d.dragging());
    CHECK(d.offset() == doctest::Approx(50.0 - tokens::touch_slop));
    d.move(-500.0, 0.03);
    CHECK(d.offset() == doctest::Approx(0.0));
}

TEST_CASE("a slow release settles on the anchor whose positional threshold was crossed") {
    Drag d = make();
    CHECK(d.compute_target(fixed_threshold + 1.0, 10.0) == Stop::middle);
    CHECK(d.compute_target(fixed_threshold - 1.0, 10.0) == Stop::low);
    CHECK(d.compute_target(300.0 - fixed_threshold - 1.0, -10.0) == Stop::low);
    CHECK(d.compute_target(300.0 - fixed_threshold + 1.0, -10.0) == Stop::middle);
}

TEST_CASE("a fast release goes to the next anchor in the direction of the fling") {
    Drag d = make();
    CHECK(d.compute_target(10.0, velocity_threshold) == Stop::middle);
    CHECK(d.compute_target(290.0, -velocity_threshold) == Stop::low);
    CHECK(d.compute_target(310.0, velocity_threshold * 4.0) == Stop::high);
}

TEST_CASE("without velocity the closest anchor wins") {
    Drag d = make();
    CHECK(d.compute_target(140.0, 0.0) == Stop::low);
    CHECK(d.compute_target(160.0, 0.0) == Stop::middle);
}

TEST_CASE("a fling settles on a spring with the release velocity and then reports the value") {
    Drag d = make();
    d.press(0.0, 0.0);
    for (int i = 1; i <= 8; ++i) {
        d.move(i * 10.0, i * 0.008);
    }
    const auto v = d.release(0.064);
    REQUIRE(v);
    CHECK(*v == doctest::Approx(1250.0).epsilon(0.01));
    const Stop target = d.fling(*v, tokens::motion_scheme.default_spatial());
    CHECK(target == Stop::middle);
    CHECK(d.target_value() == Stop::middle);
    CHECK(d.settled_value() == Stop::low);
    run(d);
    CHECK(d.settled_value() == Stop::middle);
    CHECK(d.offset() == doctest::Approx(300.0));
}

TEST_CASE("a tween ignores the velocity and lands after its duration") {
    Drag d = make();
    d.animate_to(Stop::high, motion::TweenSpec{0.256, tokens::easing::legacy}, 5000.0);
    run(d, 0.1);
    CHECK(d.offset() < 600.0);
    run(d, 0.2);
    CHECK_FALSE(d.animating());
    CHECK(d.offset() == doctest::Approx(600.0));
}

TEST_CASE("an expressive spring overshoots past the anchor before it settles") {
    Drag d = make(Stop::high);
    d.animate_to(Stop::low, tokens::motion_scheme.default_spatial(), 0.0);
    double lowest = 600.0;
    for (int i = 0; i < 120; ++i) {
        d.step(frame);
        lowest = std::min(lowest, d.offset());
    }
    CHECK(lowest < 0.0);
}

TEST_CASE("new anchors move a resting value to its new position") {
    Drag d = make(Stop::middle);
    d.set_anchors({{Stop::low, 0.0}, {Stop::middle, 250.0}, {Stop::high, 500.0}}, Stop::middle);
    CHECK(d.offset() == doctest::Approx(250.0));
    d.set_anchors({{Stop::low, 0.0}, {Stop::high, 500.0}}, Stop::high);
    CHECK(d.settled_value() == Stop::high);
    CHECK(d.offset() == doctest::Approx(500.0));
}
