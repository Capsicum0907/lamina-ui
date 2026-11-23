#include "lamina/widgets/pinch.hpp"

#include <doctest/doctest.h>

#include <optional>

using lamina::input::FingerAction;
using lamina::input::FingerEvent;
using lamina::widgets::PinchPhase;
using lamina::widgets::PinchRecognizer;
using lamina::widgets::PinchUpdate;

namespace {

constexpr lamina::widgets::PinchOptions options{.min_span = 20.0, .span_slop = 8.0};

FingerEvent finger(FingerAction action, std::uint64_t id, double x, double y) {
    return {action, id, x, y, 0.0};
}

}

TEST_CASE("one finger never pinches, and two fingers claim the touch at once") {
    PinchRecognizer p{options};
    CHECK_FALSE(p.handle(finger(FingerAction::down, 1, 100.0, 100.0)));
    CHECK_FALSE(p.handle(finger(FingerAction::move, 1, 160.0, 140.0)));
    CHECK_FALSE(p.claimed());
    CHECK_FALSE(p.active());
    CHECK_FALSE(p.handle(finger(FingerAction::down, 2, 260.0, 140.0)));
    CHECK(p.claimed());
    CHECK_FALSE(p.active());
}

TEST_CASE("a pinch begins once the span moves past the slop, at scale 1, and then follows the span and the centre") {
    PinchRecognizer p{options};
    p.handle(finger(FingerAction::down, 1, 100.0, 200.0));
    p.handle(finger(FingerAction::down, 2, 200.0, 200.0));
    CHECK_FALSE(p.handle(finger(FingerAction::move, 2, 205.0, 200.0)));
    const std::optional<PinchUpdate> began = p.handle(finger(FingerAction::move, 2, 210.0, 200.0));
    REQUIRE(began);
    CHECK(began->phase == PinchPhase::began);
    CHECK(began->scale == doctest::Approx(1.0));
    CHECK(began->focus.x == doctest::Approx(155.0));
    CHECK(began->focus.y == doctest::Approx(200.0));
    const std::optional<PinchUpdate> moved = p.handle(finger(FingerAction::move, 1, 0.0, 200.0));
    REQUIRE(moved);
    CHECK(moved->phase == PinchPhase::moved);
    CHECK(moved->scale == doctest::Approx(210.0 / 110.0));
    CHECK(moved->focus.x == doctest::Approx(105.0));
}

TEST_CASE("two fingers moving together pan without pinching") {
    PinchRecognizer p{options};
    p.handle(finger(FingerAction::down, 1, 100.0, 200.0));
    p.handle(finger(FingerAction::down, 2, 200.0, 200.0));
    for (double dy = 10.0; dy <= 200.0; dy += 10.0) {
        CHECK_FALSE(p.handle(finger(FingerAction::move, 1, 100.0, 200.0 + dy)));
        CHECK_FALSE(p.handle(finger(FingerAction::move, 2, 200.0, 200.0 + dy)));
    }
    CHECK_FALSE(p.active());
}

TEST_CASE("a finger joining or leaving a pinch moves neither its scale nor its centre") {
    PinchRecognizer p{options};
    p.handle(finger(FingerAction::down, 1, 100.0, 200.0));
    p.handle(finger(FingerAction::down, 2, 200.0, 200.0));
    p.handle(finger(FingerAction::move, 2, 260.0, 200.0));
    const std::optional<PinchUpdate> before = p.handle(finger(FingerAction::move, 1, 80.0, 200.0));
    REQUIRE(before);
    CHECK_FALSE(p.handle(finger(FingerAction::down, 3, 170.0, 400.0)));
    const std::optional<PinchUpdate> joined = p.handle(finger(FingerAction::move, 3, 170.0, 400.0));
    REQUIRE(joined);
    CHECK(joined->scale == doctest::Approx(before->scale));
    CHECK(joined->focus.x == doctest::Approx(before->focus.x));
    CHECK(joined->focus.y == doctest::Approx(before->focus.y));
    CHECK_FALSE(p.handle(finger(FingerAction::up, 3, 170.0, 400.0)));
    const std::optional<PinchUpdate> left = p.handle(finger(FingerAction::move, 1, 80.0, 200.0));
    REQUIRE(left);
    CHECK(left->scale == doctest::Approx(before->scale));
    CHECK(left->focus.x == doctest::Approx(before->focus.x));
    CHECK(left->focus.y == doctest::Approx(before->focus.y));
}

TEST_CASE("a pinch ends when fewer than two fingers are left, and the touch is claimed until the last one lifts") {
    PinchRecognizer p{options};
    p.handle(finger(FingerAction::down, 1, 100.0, 200.0));
    p.handle(finger(FingerAction::down, 2, 200.0, 200.0));
    const std::optional<PinchUpdate> moved = p.handle(finger(FingerAction::move, 2, 300.0, 200.0));
    REQUIRE(moved);
    const std::optional<PinchUpdate> ended = p.handle(finger(FingerAction::up, 1, 100.0, 200.0));
    REQUIRE(ended);
    CHECK(ended->phase == PinchPhase::ended);
    CHECK(ended->scale == doctest::Approx(moved->scale));
    CHECK_FALSE(p.active());
    CHECK(p.claimed());
    CHECK_FALSE(p.handle(finger(FingerAction::move, 2, 400.0, 300.0)));
    CHECK_FALSE(p.handle(finger(FingerAction::up, 2, 400.0, 300.0)));
    CHECK_FALSE(p.claimed());
}

TEST_CASE("a cancelled finger ends the pinch as a lift does, and cancel forgets everything") {
    PinchRecognizer p{options};
    p.handle(finger(FingerAction::down, 1, 100.0, 200.0));
    p.handle(finger(FingerAction::down, 2, 200.0, 200.0));
    p.handle(finger(FingerAction::move, 2, 300.0, 200.0));
    const std::optional<PinchUpdate> ended = p.handle(finger(FingerAction::cancel, 2, 300.0, 200.0));
    REQUIRE(ended);
    CHECK(ended->phase == PinchPhase::ended);
    p.handle(finger(FingerAction::down, 2, 300.0, 200.0));
    p.cancel();
    CHECK_FALSE(p.claimed());
    CHECK(p.fingers() == 0);
    CHECK_FALSE(p.handle(finger(FingerAction::move, 1, 0.0, 0.0)));
}
