#include <doctest/doctest.h>

#include <string>

#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/widgets/button.hpp"

using namespace lamina;
using platform::PointerAction;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;

namespace {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

widgets::Button make(ButtonStyle style, ButtonSize size, ButtonShape shape, const char* label = "Send") {
    widgets::Button b({label, style, size, shape}, fonts());
    b.set_origin({10.0, 10.0});
    return b;
}

void settle(widgets::Button& b) {
    for (int i = 0; i < 240; ++i) {
        b.step(1.0 / 60.0);
    }
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
    render::ThorVGCanvas canvas(200, 80);
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
