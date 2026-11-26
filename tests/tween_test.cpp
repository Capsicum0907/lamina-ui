#include <doctest/doctest.h>

#include <cmath>

#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/easing.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/tokens/motion.hpp"

using namespace lamina;

namespace {

constexpr double distance = 300.0;
constexpr double ran_seconds = 0.3;
constexpr double running_seconds = 1.0;
constexpr double shorter_seconds = 0.5;
constexpr double longer_seconds = 2.0;
constexpr double other_curve_seconds = 0.4;
constexpr double probe_seconds = 1e-4;
constexpr double finish_margin = 1e-3;
constexpr double continuity_tolerance = 1.0;
constexpr double eased_tolerance = 1e-6;

constexpr motion::TweenSpec running_spec{running_seconds, tokens::easing::emphasized};

motion::Tweened started() {
    motion::Tweened v(0.0);
    v.animate_to(distance, running_spec);
    v.step(ran_seconds);
    return v;
}

void check_finishes_after(motion::Tweened& v, double remaining) {
    v.step(remaining - finish_margin);
    CHECK_FALSE(v.settled());
    v.step(2.0 * finish_margin);
    CHECK(v.settled());
    CHECK(v.value() == distance);
}

void check_retimed(const motion::TweenSpec& next, double expected_progress) {
    motion::Tweened v = started();
    const double before = v.value();
    v.retime(next);
    CHECK(v.value() == before);
    CHECK(v.elapsed() / next.duration_seconds == doctest::Approx(expected_progress).epsilon(eased_tolerance));
    CHECK(motion::lerp(0.0, distance, motion::ease(next.easing, v.elapsed() / next.duration_seconds)) ==
          doctest::Approx(before).epsilon(eased_tolerance));
    motion::Tweened probe = v;
    probe.step(probe_seconds);
    CHECK(std::abs(probe.value() - before) < continuity_tolerance);
    CHECK(probe.value() >= before);
    check_finishes_after(v, next.duration_seconds * (1.0 - v.elapsed() / next.duration_seconds));
}

}

TEST_CASE("the inverse of an easing curve gives back the fraction for every token curve") {
    using namespace tokens::easing;
    for (const motion::CubicBezier& curve :
         {emphasized, emphasized_accelerate, emphasized_decelerate, standard, standard_accelerate, standard_decelerate, legacy, linear}) {
        for (double x = 0.0; x <= 1.0; x += 0.05) {
            CHECK(motion::ease(curve, motion::ease_inverse(curve, motion::ease(curve, x))) ==
                  doctest::Approx(motion::ease(curve, x)).epsilon(eased_tolerance));
        }
        CHECK(motion::ease_inverse(curve, 0.0) == 0.0);
        CHECK(motion::ease_inverse(curve, 1.0) == 1.0);
    }
}

TEST_CASE("a running tween retimed to the same curve keeps its linear progress and takes the rest of the new duration") {
    const double progress = ran_seconds / running_seconds;
    check_retimed({shorter_seconds, tokens::easing::emphasized}, progress);
    check_retimed({longer_seconds, tokens::easing::emphasized}, progress);
}

TEST_CASE("a running tween retimed to another curve goes on from the same eased value") {
    const double eased = motion::ease(running_spec.easing, ran_seconds / running_seconds);
    check_retimed({other_curve_seconds, tokens::easing::legacy}, motion::ease_inverse(tokens::easing::legacy, eased));
    check_retimed({other_curve_seconds, tokens::easing::linear}, eased);
}

TEST_CASE("retiming to the spec a tween runs with changes nothing") {
    motion::Tweened v = started();
    const double elapsed = v.elapsed();
    v.retime(running_spec);
    CHECK(v.elapsed() == elapsed);
    CHECK(v.spec() == running_spec);
}

TEST_CASE("retiming to a zero duration ends the tween, and a settled tween is not retimed") {
    motion::Tweened v = started();
    v.retime({0.0, tokens::easing::emphasized});
    CHECK(v.settled());
    CHECK(v.value() == distance);

    motion::Tweened settled(0.0);
    settled.retime({shorter_seconds, tokens::easing::legacy});
    CHECK(settled.settled());
    CHECK(settled.spec() == motion::TweenSpec{0.0, tokens::easing::linear});
}

TEST_CASE("an anchored draggable retimes a running tween and leaves a running spring to the spring retune") {
    motion::AnchoredDraggable<int> tween(0, {}, tokens::drag_gesture);
    tween.set_anchors({{0, 0.0}, {1, distance}}, 0);
    tween.animate_to(1, running_spec);
    tween.step(ran_seconds);
    const double before = tween.offset();
    tween.retune(motion::SpringSpec{1.0, 50.0});
    tween.retune(motion::TweenSpec{shorter_seconds, tokens::easing::emphasized});
    CHECK(tween.offset() == before);
    const double remaining = shorter_seconds * (1.0 - ran_seconds / running_seconds);
    tween.step(remaining - finish_margin);
    CHECK(tween.animating());
    tween.step(2.0 * finish_margin);
    CHECK_FALSE(tween.animating());
    CHECK(tween.offset() == distance);

    motion::AnchoredDraggable<int> spring(0, {}, tokens::drag_gesture);
    spring.set_anchors({{0, 0.0}, {1, distance}}, 0);
    spring.animate_to(1, motion::SpringSpec{1.0, 50.0});
    spring.step(ran_seconds);
    motion::AnchoredDraggable<int> untouched = spring;
    spring.retune(motion::TweenSpec{shorter_seconds, tokens::easing::emphasized});
    spring.step(ran_seconds);
    untouched.step(ran_seconds);
    CHECK(spring.offset() == untouched.offset());
}
