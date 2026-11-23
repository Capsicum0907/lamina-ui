#include <doctest/doctest.h>

#include <vector>

#include "lamina/input/touch_release.hpp"
#include "lamina/widgets/interaction.hpp"

using lamina::input::FingerAction;
using lamina::input::FingerEvent;
using lamina::input::PointerAction;
using lamina::input::PointerEvent;
using lamina::input::PointerKind;
using lamina::input::TouchReleaseGate;

namespace {

constexpr std::uint64_t first_finger = 7;
constexpr std::uint64_t second_finger = 8;
constexpr double edge_x = 3.0;
constexpr double release_x = 60.0;
constexpr double row_y = 200.0;

PointerEvent touch(PointerAction action, double x, double y) {
    return {action, x, y, 1, PointerKind::touch};
}

FingerEvent finger(FingerAction action, std::uint64_t id, double x, double y) {
    return {action, id, x, y};
}

std::vector<PointerAction> actions(const std::vector<PointerEvent>& events) {
    std::vector<PointerAction> out;
    for (const PointerEvent& e : events) {
        out.push_back(e.action);
    }
    return out;
}

std::vector<PointerEvent> press(TouchReleaseGate& gate, std::uint64_t id, double x, double y) {
    std::vector<PointerEvent> out = gate.pointer(touch(PointerAction::move, x, y));
    for (const PointerEvent& e : gate.pointer(touch(PointerAction::down, x, y))) {
        out.push_back(e);
    }
    for (const PointerEvent& e : gate.finger(finger(FingerAction::down, id, x, y))) {
        out.push_back(e);
    }
    return out;
}

}

TEST_CASE("a lifted finger releases the held up and then leaves") {
    TouchReleaseGate gate;
    CHECK(actions(press(gate, first_finger, release_x, row_y)) == std::vector{PointerAction::move, PointerAction::down});
    CHECK(gate.pointer(touch(PointerAction::up, release_x, row_y)).empty());
    CHECK(gate.holding());
    const std::vector<PointerEvent> out = gate.finger(finger(FingerAction::up, first_finger, release_x, row_y));
    CHECK(actions(out) == std::vector{PointerAction::up, PointerAction::leave});
    CHECK(out[0].x_dp == release_x);
    CHECK(out[1].kind == PointerKind::touch);
    CHECK_FALSE(gate.holding());
}

TEST_CASE("a cancelled finger turns the held up into a leave without an up") {
    TouchReleaseGate gate;
    press(gate, first_finger, edge_x, row_y);
    gate.pointer(touch(PointerAction::move, release_x, row_y));
    CHECK(gate.pointer(touch(PointerAction::up, release_x, row_y)).empty());
    const std::vector<PointerEvent> out = gate.finger(finger(FingerAction::cancel, first_finger, release_x, row_y));
    CHECK(actions(out) == std::vector{PointerAction::leave});
    CHECK(out[0].clicks == 0);
    CHECK_FALSE(gate.holding());
}

TEST_CASE("a cancel that arrives before the up also drops the up") {
    TouchReleaseGate gate;
    press(gate, first_finger, edge_x, row_y);
    CHECK(gate.finger(finger(FingerAction::cancel, first_finger, release_x, row_y)).empty());
    CHECK(actions(gate.pointer(touch(PointerAction::up, release_x, row_y))) == std::vector{PointerAction::leave});
    CHECK_FALSE(gate.holding());
}

TEST_CASE("a lift that arrives before the up lets the up through at once") {
    TouchReleaseGate gate;
    press(gate, first_finger, release_x, row_y);
    CHECK(gate.finger(finger(FingerAction::up, first_finger, release_x, row_y)).empty());
    CHECK(actions(gate.pointer(touch(PointerAction::up, release_x, row_y))) == std::vector{PointerAction::up, PointerAction::leave});
    CHECK_FALSE(gate.holding());
}

TEST_CASE("only the first finger, the one the pointer follows, decides") {
    TouchReleaseGate gate;
    press(gate, first_finger, release_x, row_y);
    CHECK(gate.finger(finger(FingerAction::down, second_finger, release_x, row_y)).empty());
    CHECK(gate.finger(finger(FingerAction::cancel, second_finger, release_x, row_y)).empty());
    gate.pointer(touch(PointerAction::up, release_x, row_y));
    CHECK(actions(gate.finger(finger(FingerAction::up, first_finger, release_x, row_y))) == std::vector{PointerAction::up, PointerAction::leave});
}

TEST_CASE("an up whose finger event never comes is released after one frame") {
    TouchReleaseGate gate;
    gate.pointer(touch(PointerAction::down, release_x, row_y));
    gate.pointer(touch(PointerAction::up, release_x, row_y));
    CHECK(gate.end_frame().empty());
    CHECK(gate.holding());
    CHECK(actions(gate.end_frame()) == std::vector{PointerAction::up, PointerAction::leave});
    CHECK_FALSE(gate.holding());
}

TEST_CASE("a new touch releases an up that is still held") {
    TouchReleaseGate gate;
    gate.pointer(touch(PointerAction::down, release_x, row_y));
    gate.pointer(touch(PointerAction::up, release_x, row_y));
    CHECK(actions(gate.pointer(touch(PointerAction::down, edge_x, row_y))) ==
          std::vector{PointerAction::up, PointerAction::leave, PointerAction::down});
}

TEST_CASE("mouse and pen events pass unchanged") {
    TouchReleaseGate gate;
    for (const PointerKind kind : {PointerKind::mouse, PointerKind::pen}) {
        const PointerEvent up{PointerAction::up, release_x, row_y, 1, kind};
        const std::vector<PointerEvent> out = gate.pointer(up);
        CHECK(actions(out) == std::vector{PointerAction::up});
        CHECK(out[0].kind == kind);
    }
    CHECK_FALSE(gate.holding());
}

TEST_CASE("a press that the system cancels never clicks") {
    TouchReleaseGate gate;
    lamina::widgets::PressState state;
    const lamina::render::Rect cell{0.0, 150.0, 120.0, 120.0};
    for (const PointerEvent& e : press(gate, first_finger, edge_x, row_y)) {
        state.update(e, cell);
    }
    CHECK(state.pressed);
    bool clicked = false;
    for (const PointerEvent& e : gate.pointer(touch(PointerAction::up, release_x, row_y))) {
        clicked = state.update(e, cell) || clicked;
    }
    for (const PointerEvent& e : gate.finger(finger(FingerAction::cancel, first_finger, release_x, row_y))) {
        clicked = state.update(e, cell) || clicked;
    }
    CHECK_FALSE(clicked);
    CHECK_FALSE(state.pressed);
}
