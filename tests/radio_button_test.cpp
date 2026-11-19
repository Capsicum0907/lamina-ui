#include <doctest/doctest.h>

#include "lamina/widgets/radio_button.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

TEST_CASE("radio button tokens and selection") {
    CHECK(tokens::radio_button_tokens.icon_size == 20.0);
    CHECK(tokens::radio_button_tokens.dot_size == 12.0);
    widgets::RadioButton r;
    CHECK_FALSE(r.selected());
    CHECK(click(r, 20.0, 20.0));
    CHECK(r.selected());
    CHECK(click(r, 20.0, 20.0));
    CHECK(r.selected());
    settle(r);
    CHECK(r.dot_radius() == doctest::Approx(6.0));
    r.set_selected(false);
    settle(r);
    CHECK(r.dot_radius() == doctest::Approx(0.0).epsilon(0.01));
}

TEST_CASE("a selected radio button paints a ring and a dot") {
    widgets::RadioButton r({true});
    thorvg::Canvas canvas(40, 40);
    canvas.clear(0xffffffff);
    r.paint(canvas, scheme());
    const color::Argb primary = scheme()[color::Role::primary];
    CHECK(canvas.pixel(20, 20) == primary);
    CHECK(canvas.pixel(20, 11) == primary);
    CHECK(canvas.pixel(20, 13) == 0xffffffff);
}
