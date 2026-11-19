#include <doctest/doctest.h>

#include "lamina/widgets/switch.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

TEST_CASE("switch tokens") {
    const auto& t = tokens::switch_tokens;
    CHECK(t.track_width == 52.0);
    CHECK(t.track_height == 32.0);
    CHECK(t.unchecked_thumb == 16.0);
    CHECK(t.checked_thumb == 24.0);
    CHECK(t.pressed_thumb == 28.0);
    CHECK(t.checked.track == color::Role::primary);
    CHECK(t.unchecked.thumb == color::Role::outline);
    CHECK_FALSE(t.checked.border.has_value());
}

TEST_CASE("a switch toggles on click and moves its thumb to the end") {
    widgets::Switch s;
    s.set_origin({0.0, 8.0});
    CHECK_FALSE(s.checked());
    CHECK(s.thumb_center_x() == 16.0);
    CHECK(click(s, 26.0, 24.0));
    CHECK(s.checked());
    CHECK(s.animating());
    settle(s);
    CHECK(s.thumb_center_x() == doctest::Approx(36.0));
    CHECK(s.thumb_size() == doctest::Approx(24.0));
    CHECK(click(s, 26.0, 24.0));
    CHECK_FALSE(s.checked());
    settle(s);
    CHECK(s.thumb_center_x() == doctest::Approx(16.0));
    CHECK(s.thumb_size() == doctest::Approx(16.0));
}

TEST_CASE("the switch thumb grows while pressed and with an icon") {
    widgets::Switch s;
    s.handle({PointerAction::down, 10.0, 16.0});
    settle(s);
    CHECK(s.thumb_size() == doctest::Approx(28.0));
    s.handle({PointerAction::leave, 0.0, 0.0});
    settle(s);
    CHECK(s.thumb_size() == doctest::Approx(16.0));

    widgets::Switch with_icon({false, std::nullopt, icons::Icon{icon_font(), icons::Symbol::close}});
    CHECK(with_icon.thumb_size() == doctest::Approx(24.0));
}

TEST_CASE("a checked switch paints a primary track and its check icon") {
    widgets::Switch s({true, icons::Icon{icon_font(), icons::Symbol::check}});
    thorvg::Canvas canvas(52, 32);
    canvas.clear(0xffffffff);
    s.paint(canvas, scheme());
    CHECK(canvas.pixel(8, 16) == scheme()[color::Role::primary]);
    const int thumb_with_icon = count_color(canvas, scheme()[color::Role::on_primary]);
    CHECK(thumb_with_icon > 50);
    widgets::Switch plain({true});
    canvas.clear(0xffffffff);
    plain.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[color::Role::on_primary]) > thumb_with_icon + 20);
}
