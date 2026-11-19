#include <doctest/doctest.h>

#include "lamina/widgets/checkbox.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::CheckboxState;

TEST_CASE("checkbox tokens follow Checkbox.kt") {
    const auto& t = tokens::checkbox_tokens;
    CHECK(t.box_size == 20.0);
    CHECK(t.stroke_width == 2.0);
    CHECK(t.corner_radius == 2.0);
    CHECK(t.uncheck_snap_delay_seconds == doctest::Approx(0.1));
    CHECK(t.check_cross.y == doctest::Approx(0.7));
}

TEST_CASE("a checkbox toggles between off and on, and indeterminate goes to on") {
    widgets::Checkbox c;
    c.set_origin({0.0, 0.0});
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::on);
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::off);
    c.set_state(CheckboxState::indeterminate);
    CHECK(click(c, 20.0, 20.0));
    CHECK(c.state() == CheckboxState::on);
    CHECK_FALSE(click(c, 200.0, 200.0));
}

TEST_CASE("the check mark draws in, and snaps away after the delay") {
    widgets::Checkbox c;
    CHECK(c.check_fraction() == 0.0);
    c.set_state(CheckboxState::on);
    c.step(1.0 / 60.0);
    const double early = c.check_fraction();
    CHECK(early > 0.0);
    CHECK(early < 1.0);
    settle(c);
    CHECK(c.check_fraction() == doctest::Approx(1.0));
    c.set_state(CheckboxState::off);
    c.step(0.05);
    CHECK(c.check_fraction() == doctest::Approx(1.0));
    c.step(0.06);
    CHECK(c.check_fraction() == 0.0);
}

TEST_CASE("the check mark path grows with the draw fraction") {
    const render::Rect box{0.0, 0.0, 20.0, 20.0};
    CHECK(widgets::check_mark_path(box, 0.0, 0.0, 2.0).empty());
    thorvg::Canvas canvas(20, 20);
    canvas.clear(0xffffffff);
    canvas.fill_path(widgets::check_mark_path(box, 0.3, 0.0, 2.0), 0xff000000);
    const int partial = count_color(canvas, 0xff000000);
    canvas.clear(0xffffffff);
    canvas.fill_path(widgets::check_mark_path(box, 1.0, 0.0, 2.0), 0xff000000);
    const int full = count_color(canvas, 0xff000000);
    CHECK(partial > 0);
    CHECK(full > partial * 2);
    CHECK(canvas.pixel(8, 13) == 0xff000000);
}

TEST_CASE("a checked checkbox fills its box with primary and draws the check in on-primary") {
    widgets::Checkbox c({CheckboxState::on});
    thorvg::Canvas canvas(40, 40);
    canvas.clear(0xffffffff);
    c.paint(canvas, scheme());
    CHECK(canvas.pixel(12, 12) == scheme()[color::Role::primary]);
    CHECK(count_color(canvas, scheme()[color::Role::on_primary]) > 10);

    widgets::Checkbox off;
    canvas.clear(0xffffffff);
    off.paint(canvas, scheme());
    CHECK(canvas.pixel(20, 20) == 0xffffffff);
    CHECK(canvas.pixel(10, 20) == scheme()[color::Role::on_surface_variant]);
}
