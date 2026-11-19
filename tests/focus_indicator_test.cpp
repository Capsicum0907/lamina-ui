#include <doctest/doctest.h>

#include "lamina/tokens/focus.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::KeyActivation;

TEST_CASE("Space activates; Enter only where the widget takes it; never with Ctrl or on auto-repeat") {
    CHECK(widgets::activates({Key::space, false}, KeyActivation::space));
    CHECK_FALSE(widgets::activates({Key::enter, false}, KeyActivation::space));
    CHECK(widgets::activates({Key::enter, false}, KeyActivation::space_or_enter));
    CHECK(widgets::activates({Key::space, true}, KeyActivation::space_or_enter));
    CHECK_FALSE(widgets::activates({Key::space, false, true}, KeyActivation::space_or_enter));
    CHECK_FALSE(widgets::activates({Key::enter, false, false, true}, KeyActivation::space_or_enter));
    CHECK_FALSE(widgets::activates({Key::a, false}, KeyActivation::space_or_enter));
}

TEST_CASE("the indicator activates only while focused, and a pointer press anywhere takes the focus away") {
    widgets::FocusIndicator f;
    CHECK_FALSE(f.activates({Key::space, false}, KeyActivation::space));
    f.set_focused(true);
    CHECK(f.activates({Key::space, false}, KeyActivation::space));
    f.release_on_press({PointerAction::move, 1.0, 1.0});
    CHECK(f.focused());
    f.release_on_press({PointerAction::down, 500.0, 500.0});
    CHECK_FALSE(f.focused());
}

TEST_CASE("focus raises the state layer to the focus opacity without lowering a pressed one") {
    widgets::FocusIndicator f;
    CHECK(f.state_layer_opacity(0.0) == 0.0);
    CHECK(f.state_layer_opacity(tokens::state_layer_opacity::hover) == tokens::state_layer_opacity::hover);
    f.set_focused(true);
    CHECK(f.state_layer_opacity(0.0) == tokens::state_layer_opacity::focus);
    CHECK(f.state_layer_opacity(0.5) == 0.5);
}

TEST_CASE("the ring is drawn outside the shape at the outer offset, in secondary, and fades in and out") {
    const tokens::FocusIndicatorTokens& t = tokens::focus_indicator;
    const render::Rect shape{20.0, 20.0, 60.0, 40.0};
    widgets::FocusIndicator f;
    thorvg::Canvas canvas(100, 80);
    auto draw = [&] {
        canvas.clear(0xffffffff);
        f.paint(canvas, scheme(), shape, 0.0);
    };
    draw();
    CHECK(count_color(canvas, 0xffffffff) == 100 * 80);
    CHECK(f.settled());

    f.set_focused(true);
    CHECK_FALSE(f.settled());
    settle(f);
    CHECK(f.amount() == doctest::Approx(1.0));
    draw();
    const color::Argb ring = scheme()[t.color];
    const int mid_y = 40;
    CHECK(canvas.pixel(static_cast<int>(shape.x - t.outer_offset - t.thickness / 2.0), mid_y) == ring);
    CHECK(canvas.pixel(static_cast<int>(shape.x - t.outer_offset / 2.0), mid_y) == 0xffffffff);
    CHECK(canvas.pixel(static_cast<int>(shape.x + 1.0), mid_y) == 0xffffffff);
    CHECK(canvas.pixel(static_cast<int>(shape.x - t.outer_offset - t.thickness - 1.0), mid_y) == 0xffffffff);

    f.set_focused(false);
    settle(f);
    draw();
    CHECK(count_color(canvas, 0xffffffff) == 100 * 80);
}
