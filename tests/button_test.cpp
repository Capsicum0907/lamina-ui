#include <doctest/doctest.h>

#include "lamina/widgets/button.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;

namespace {

widgets::Button make(ButtonStyle style, ButtonSize size, ButtonShape shape, const char* label = "Send") {
    widgets::Button b({label, style, size, shape}, fonts());
    b.set_origin({10.0, 10.0});
    return b;
}

}

TEST_CASE("button height follows the size tokens") {
    CHECK(make(ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round).height() == 32.0);
    CHECK(make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round).height() == 40.0);
    CHECK(make(ButtonStyle::filled, ButtonSize::medium, ButtonShape::round).height() == 56.0);
    CHECK(make(ButtonStyle::filled, ButtonSize::large, ButtonShape::round).height() == 96.0);
    CHECK(make(ButtonStyle::filled, ButtonSize::extra_large, ButtonShape::round).height() == 136.0);
}

TEST_CASE("button width is padding plus label, but never below the minimum") {
    const auto narrow = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round, "A");
    CHECK(narrow.width() == tokens::button_min_width);
    const auto wide = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round, "A much longer label");
    CHECK(wide.width() > tokens::button_min_width);
}

TEST_CASE("a click needs press and release inside the button") {
    auto b = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round);
    CHECK_FALSE(b.handle({PointerAction::down, 20.0, 20.0}));
    CHECK(b.handle({PointerAction::up, 21.0, 21.0}));
    CHECK_FALSE(b.handle({PointerAction::down, 20.0, 20.0}));
    CHECK_FALSE(b.handle({PointerAction::up, 500.0, 500.0}));
    CHECK_FALSE(b.handle({PointerAction::down, 500.0, 500.0}));
    CHECK_FALSE(b.handle({PointerAction::up, 20.0, 20.0}));
}

TEST_CASE("pressing a round button squares its corners without bouncing") {
    auto b = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round);
    thorvg::Canvas canvas(200, 80);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(13, 13) == 0xffffffff);

    b.handle({PointerAction::down, 20.0, 20.0});
    CHECK(b.animating());
    settle(b);
    CHECK_FALSE(b.animating());
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(13, 13) != 0xffffffff);
}

TEST_CASE("cancel_interaction drops a press, so the release no longer clicks, and the corners spring back") {
    auto b = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round);
    thorvg::Canvas canvas(200, 80);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    b.handle({PointerAction::down, 20.0, 20.0});
    settle(b);
    b.cancel_interaction();
    CHECK(b.animating());
    settle(b);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme);
    CHECK(canvas.pixel(13, 13) == 0xffffffff);
    CHECK_FALSE(b.handle({PointerAction::up, 20.0, 20.0}));
}

TEST_CASE("set_size gives a button any size, and a round button's corners follow the new height at once") {
    auto b = make(ButtonStyle::filled, ButtonSize::large, ButtonShape::round, "7");
    b.set_size(120.0, 50.0);
    CHECK(b.width() == 120.0);
    CHECK(b.height() == 50.0);
    CHECK(b.bounds().width == 120.0);
    CHECK(b.bounds().height == 50.0);
    CHECK_FALSE(b.animating());

    thorvg::Canvas canvas(200, 80);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme());
    CHECK(canvas.pixel(12, 12) == 0xffffffff);
    CHECK(canvas.pixel(35, 12) != 0xffffffff);
    CHECK(canvas.pixel(70, 59) != 0xffffffff);
    CHECK(canvas.pixel(70, 62) == 0xffffffff);
}

TEST_CASE("set_size during a press keeps the corners springing toward the pressed shape") {
    auto b = make(ButtonStyle::filled, ButtonSize::large, ButtonShape::round, "7");
    b.handle({PointerAction::down, 40.0, 40.0});
    b.set_size(120.0, 80.0);
    CHECK(b.animating());
    settle(b);
    CHECK_FALSE(b.animating());
}

TEST_CASE("an icon-only button leaves no gap for the missing label") {
    widgets::Button b({"", ButtonStyle::tonal, ButtonSize::large, ButtonShape::round, icons::Icon{icon_font(), res::icons::backspace}}, fonts());
    const auto& size = tokens::button_size(ButtonSize::large);
    CHECK(b.width() == doctest::Approx(size.leading_space + size.icon_size + size.trailing_space));
}

TEST_CASE("ButtonOptions::colors replaces the style's container and content roles") {
    widgets::ButtonOptions options{"", ButtonStyle::text, ButtonSize::small, ButtonShape::square};
    options.colors = widgets::ButtonColors{color::Role::tertiary_container, color::Role::on_tertiary_container};
    widgets::Button b(options, fonts());
    b.set_origin({10.0, 10.0});
    thorvg::Canvas canvas(200, 80);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme());
    CHECK(canvas.pixel(40, 30) == scheme()[color::Role::tertiary_container]);
}

TEST_CASE("ButtonColors without a container keeps the button transparent") {
    widgets::ButtonOptions options{"Undo", ButtonStyle::text, ButtonSize::small, ButtonShape::square};
    options.colors = widgets::ButtonColors{std::nullopt, color::Role::inverse_primary};
    widgets::Button b(options, fonts());
    b.set_origin({10.0, 10.0});
    thorvg::Canvas canvas(200, 80);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme());
    CHECK(canvas.pixel(12, 12) == 0xffffffff);
    CHECK(count_color(canvas, scheme()[color::Role::inverse_primary]) > 0);
    b.handle(input::PointerEvent{input::PointerAction::move, 20.0, 20.0});
    settle(b);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme());
    CHECK(canvas.pixel(12, 30) != 0xffffffff);
}

TEST_CASE("ButtonColors can give the focus ring its own color role") {
    widgets::ButtonOptions options{"Undo", ButtonStyle::text, ButtonSize::small, ButtonShape::round};
    options.colors = widgets::ButtonColors{std::nullopt, color::Role::inverse_primary, color::Role::tertiary};
    widgets::Button b(options, fonts());
    b.set_origin({20.0, 20.0});
    b.set_focused(true);
    settle(b);
    thorvg::Canvas canvas(200, 100);
    canvas.clear(0xffffffff);
    b.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[color::Role::tertiary]) > 0);
    CHECK(count_color(canvas, scheme()[tokens::focus_indicator.color]) == 0);
}

TEST_CASE("set_label relays out the label and refits an unsized button") {
    auto b = make(ButtonStyle::filled, ButtonSize::small, ButtonShape::round, "A");
    const double narrow = b.width();
    b.set_label("A much longer label");
    CHECK(b.width() > narrow);
    b.set_size(80.0, 40.0);
    b.set_label("B");
    CHECK(b.width() == 80.0);
}

TEST_CASE("set_content_scale scales the drawn label without changing the size") {
    thorvg::Canvas canvas(300, 200);
    auto ink = [&](double scale) {
        widgets::Button b({"88", ButtonStyle::text, ButtonSize::small, ButtonShape::round}, fonts());
        b.set_size(200.0, 150.0);
        b.set_origin({10.0, 10.0});
        b.set_content_scale(scale, scale);
        canvas.clear(0xffffffff);
        b.paint(canvas, scheme());
        CHECK(b.width() == 200.0);
        return 300 * 200 - count_color(canvas, 0xffffffff);
    };
    const int normal = ink(1.0);
    const int doubled = ink(2.0);
    CHECK(doubled > normal * 3);
}

TEST_CASE("set_shape morphs a round button to its square corners on a spring and back") {
    widgets::Button b({"", ButtonStyle::filled, ButtonSize::large, ButtonShape::round}, fonts());
    b.set_size(96.0, 96.0);
    b.set_origin({10.0, 10.0});
    thorvg::Canvas canvas(140, 140);
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    auto corner_filled = [&] {
        canvas.clear(0xffffffff);
        b.paint(canvas, scheme);
        return canvas.pixel(12, 30) != 0xffffffff;
    };
    CHECK_FALSE(corner_filled());
    b.set_shape(ButtonShape::square);
    CHECK(b.animating());
    settle(b);
    CHECK_FALSE(b.animating());
    CHECK(corner_filled());
    b.set_shape(ButtonShape::round);
    settle(b);
    CHECK_FALSE(corner_filled());
}

TEST_CASE("set_icon cross-fades to the new icon and keeps the size of a sized button") {
    widgets::Button b({"", ButtonStyle::filled, ButtonSize::large, ButtonShape::round, icons::Icon{icon_font(), res::icons::play_arrow}},
                      fonts());
    b.set_size(96.0, 96.0);
    b.set_icon(icons::Icon{icon_font(), res::icons::pause});
    CHECK(b.animating());
    CHECK(b.width() == 96.0);
    settle(b);
    CHECK_FALSE(b.animating());
    b.set_icon(icons::Icon{icon_font(), res::icons::pause});
    CHECK_FALSE(b.animating());
}

TEST_CASE("ButtonOptions::icon_style draws the icon with other symbol axes") {
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    auto ink = [&](double fill) {
        widgets::ButtonOptions o{"", ButtonStyle::text, ButtonSize::large, ButtonShape::round, icons::Icon{icon_font(), res::icons::pause}};
        o.icon_style.fill = fill;
        widgets::Button b(o, fonts());
        thorvg::Canvas canvas(140, 120);
        canvas.clear(0xffffffff);
        b.paint(canvas, scheme);
        return count_color(canvas, scheme[color::Role::primary]);
    };
    CHECK(ink(1.0) > ink(0.0));
}
