#include <doctest/doctest.h>

#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/popup_transition.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

namespace {

widgets::PopupTransition make() {
    return widgets::PopupTransition(0.8, tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast),
                                    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast));
}

}

TEST_CASE("a popup transition starts hidden and snaps to the closed scale only when it opens from hidden") {
    auto p = make();
    CHECK_FALSE(p.is_open());
    CHECK_FALSE(p.visible());
    CHECK(p.scale() == doctest::Approx(0.8));
    CHECK(p.opacity() == 0.0);
    CHECK(p.open());
    CHECK(p.is_open());
    CHECK(p.visible());
    CHECK_FALSE(p.settled());
    settle(p);
    CHECK(p.settled());
    CHECK(p.scale() == doctest::Approx(1.0));
    CHECK(p.opacity() == doctest::Approx(1.0));
    p.close();
    p.step(frame_seconds);
    CHECK(p.visible());
    CHECK_FALSE(p.open());
    CHECK(p.scale() > 0.8);
    p.close();
    settle(p);
    CHECK_FALSE(p.visible());
    CHECK(p.scale() == doctest::Approx(0.8));
}

TEST_CASE("a popup transition is dismissed by a press outside its bounds or by Escape, only while open") {
    auto p = make();
    const render::Rect bounds{10.0, 10.0, 100.0, 50.0};
    CHECK_FALSE(p.dismisses({PointerAction::down, 0.0, 0.0}, bounds));
    CHECK_FALSE(p.dismisses(input::KeyEvent{input::Key::escape, false}));
    p.open();
    CHECK(p.dismisses({PointerAction::down, 0.0, 0.0}, bounds));
    CHECK_FALSE(p.dismisses({PointerAction::down, 20.0, 20.0}, bounds));
    CHECK_FALSE(p.dismisses({PointerAction::move, 0.0, 0.0}, bounds));
    CHECK_FALSE(p.dismisses({PointerAction::up, 0.0, 0.0}, bounds));
    CHECK(p.dismisses(input::KeyEvent{input::Key::escape, false}));
}
