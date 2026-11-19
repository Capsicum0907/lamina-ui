#include <doctest/doctest.h>

#include <cstdlib>
#include <string>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;
using input::PointerAction;

namespace {

const text::FontStack& fonts() {
    static const text::FontStack stack({text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf")});
    return stack;
}

const icons::IconFont& icon_font() {
    static const icons::IconFont font = icons::IconFont::from_directory(LAMINA_ICONS_DIR);
    return font;
}

const color::ColorScheme& scheme() {
    static const color::ColorScheme s = color::spec_2021().build(color::default_scheme_options);
    return s;
}

void settle(widgets::Menu& m) {
    for (int i = 0; i < 240; ++i) {
        m.step(1.0 / 60.0);
    }
}

widgets::Menu make() {
    return widgets::Menu({{{"Edit", icons::Icon{icon_font(), icons::Symbol::edit}}, {"Share"}, {"Delete"}}}, fonts());
}

constexpr layout::Size window{400.0, 600.0};
constexpr render::Rect anchor{20.0, 40.0, 40.0, 40.0};

}

TEST_CASE("menu tokens follow MenuTokens.kt and Menu.kt") {
    const auto& t = tokens::menu_tokens;
    CHECK(t.container == color::Role::surface_container);
    CHECK(t.corner == tokens::CornerSize::extra_small);
    CHECK(tokens::elevation_dp[t.elevation] == 3.0);
    CHECK(t.vertical_padding == 8.0);
    CHECK(t.item_height == 48.0);
    CHECK(t.item_min_width == 112.0);
    CHECK(t.item_max_width == 280.0);
    CHECK(t.window_vertical_margin == 48.0);
    CHECK(t.closed_scale == 0.8);
}

TEST_CASE("the menu opens below its anchor, or above it when there is no room below") {
    auto m = make();
    CHECK(m.width() == 112.0);
    CHECK(m.height() == 160.0);
    CHECK(widgets::menu_position(anchor, window, {112.0, 160.0}).x == 20.0);
    CHECK(widgets::menu_position(anchor, window, {112.0, 160.0}).y == 80.0);
    const render::Rect low{300.0, 500.0, 40.0, 40.0};
    const render::Point above = widgets::menu_position(low, window, {112.0, 160.0});
    CHECK(above.y == 340.0);
    CHECK(above.x == 228.0);
    const render::Rect mid{20.0, 120.0, 40.0, 40.0};
    CHECK(widgets::menu_position(mid, {400.0, 300.0}, {112.0, 160.0}).y == 48.0);
}

TEST_CASE("the menu expands from the anchor and fades in") {
    auto m = make();
    CHECK_FALSE(m.visible());
    m.open(anchor, window);
    CHECK(m.is_open());
    CHECK(m.scale() == doctest::Approx(0.8));
    CHECK(m.opacity() == 0.0);
    CHECK(m.transform_origin().x == doctest::Approx(40.0));
    CHECK(m.transform_origin().y == doctest::Approx(80.0));
    m.step(1.0 / 60.0);
    CHECK(m.scale() > 0.8);
    CHECK(m.opacity() > 0.0);
    settle(m);
    CHECK(m.scale() == doctest::Approx(1.0));
    CHECK(m.opacity() == doctest::Approx(1.0));
}

TEST_CASE("choosing an item reports it and closes the menu") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    const render::Rect item = m.item_rect(1);
    auto down = m.handle({PointerAction::down, item.x + 10.0, item.y + 10.0});
    CHECK(down.consumed);
    CHECK_FALSE(down.chosen);
    auto up = m.handle({PointerAction::up, item.x + 10.0, item.y + 10.0});
    REQUIRE(up.chosen);
    CHECK(*up.chosen == 1);
    CHECK_FALSE(m.is_open());
    CHECK(m.visible());
    settle(m);
    CHECK_FALSE(m.visible());
    CHECK_FALSE(m.handle({PointerAction::down, item.x + 10.0, item.y + 10.0}).consumed);
}

TEST_CASE("a press outside the menu or Escape closes it without choosing") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    auto outside = m.handle({PointerAction::down, 350.0, 500.0});
    CHECK(outside.consumed);
    CHECK_FALSE(outside.chosen);
    CHECK_FALSE(m.is_open());
    m.open(anchor, window);
    CHECK(m.handle(input::KeyEvent{input::Key::escape, false}));
    CHECK_FALSE(m.is_open());
}

TEST_CASE("an open menu paints its container and the hovered item's state layer") {
    auto m = make();
    m.open({0.0, 0.0, 10.0, 0.0}, {200.0, 200.0});
    settle(m);
    thorvg::Canvas canvas(200, 200);
    canvas.clear(0xffffffff);
    m.paint(canvas, scheme());
    const color::Argb container = scheme()[color::Role::surface_container];
    CHECK(m.bounds().y == 20.0);
    CHECK(canvas.pixel(100, 24) == container);
    m.handle({PointerAction::move, 100.0, 90.0});
    settle(m);
    canvas.clear(0xffffffff);
    m.paint(canvas, scheme());
    const color::Argb hovered = canvas.pixel(100, 90);
    CHECK(hovered != container);
    const color::Argb expected = color::overlay(container, scheme()[color::Role::on_surface], tokens::state_layer_opacity::hover);
    for (int shift : {0, 8, 16}) {
        CHECK(std::abs(static_cast<int>((hovered >> shift) & 0xff) - static_cast<int>((expected >> shift) & 0xff)) <= 2);
    }
}
