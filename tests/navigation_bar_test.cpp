#include <doctest/doctest.h>

#include <string>

#include "lamina/widgets/navigation_bar.hpp"
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

void settle(widgets::NavigationBar& bar) {
    for (int i = 0; i < 240; ++i) {
        bar.step(1.0 / 60.0);
    }
}

widgets::NavigationBar make() {
    std::vector<widgets::NavigationBarItem> items;
    for (auto [symbol, label] : {std::pair{icons::Symbol::home, "Home"}, std::pair{icons::Symbol::search, "Search"},
                                 std::pair{icons::Symbol::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    widgets::NavigationBar bar({items}, fonts());
    bar.set_width(360.0);
    return bar;
}

bool click(widgets::NavigationBar& bar, double x, double y) {
    bar.handle({PointerAction::down, x, y});
    const bool changed = bar.handle({PointerAction::up, x, y});
    bar.handle({PointerAction::leave, 0.0, 0.0});
    return changed;
}

}

TEST_CASE("navigation bar tokens follow NavigationBarTokens.kt and ShortNavigationBar.kt") {
    const auto& t = tokens::navigation_bar_tokens;
    CHECK(t.height == 64.0);
    CHECK(t.indicator_width == 56.0);
    CHECK(t.indicator_height == 32.0);
    CHECK(t.item_vertical_padding == 6.0);
    CHECK(t.indicator_to_label_space == 4.0);
    CHECK(t.container == color::Role::surface_container);
    CHECK(t.indicator == color::Role::secondary_container);
    CHECK(t.active_label == color::Role::secondary);
    CHECK(t.label == tokens::TypeRole::label_medium);
}

TEST_CASE("the navigation bar shares its width equally and places the pill 6 dp from the top") {
    auto bar = make();
    CHECK(bar.height() == 64.0);
    CHECK(bar.item_rect(1).x == doctest::Approx(120.0));
    CHECK(bar.item_rect(1).width == doctest::Approx(120.0));
    const render::Rect pill = bar.indicator_rect(0);
    CHECK(pill.width == doctest::Approx(56.0));
    CHECK(pill.y == doctest::Approx(6.0));
    CHECK(pill.x == doctest::Approx(32.0));
    CHECK(bar.indicator_rect(1).width == 0.0);
}

TEST_CASE("selecting an item grows its pill from the center on the spatial spring") {
    auto bar = make();
    CHECK(click(bar, 180.0, 30.0));
    CHECK(bar.selected() == 1);
    bar.step(0.05);
    const render::Rect growing = bar.indicator_rect(1);
    CHECK(growing.width > 0.0);
    CHECK(growing.width < 56.0);
    CHECK(growing.x + growing.width / 2.0 == doctest::Approx(180.0));
    double peak = 0.0;
    for (int i = 0; i < 240; ++i) {
        bar.step(1.0 / 240.0);
        peak = std::max(peak, bar.indicator_progress(1));
    }
    CHECK(peak > 1.0);
    settle(bar);
    CHECK(bar.indicator_rect(1).width == doctest::Approx(56.0));
    CHECK(bar.indicator_rect(0).width == doctest::Approx(0.0));
    CHECK_FALSE(click(bar, 180.0, 30.0));
    CHECK_FALSE(click(bar, 180.0, 90.0));
}

TEST_CASE("the navigation bar paints the selected pill on the container") {
    auto bar = make();
    thorvg::Canvas canvas(360, 64);
    canvas.clear(0xffffffff);
    bar.paint(canvas, scheme());
    CHECK(canvas.pixel(36, 22) == scheme()[color::Role::secondary_container]);
    CHECK(canvas.pixel(150, 22) == scheme()[color::Role::surface_container]);
    CHECK(canvas.pixel(156, 22) == scheme()[color::Role::surface_container]);
}
