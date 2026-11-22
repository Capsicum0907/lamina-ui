#include <doctest/doctest.h>

#include <cmath>

#include "lamina/widgets/navigation_bar.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

namespace {

widgets::NavigationBar make() {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{res::icons::home, "Home"}, std::pair{res::icons::search, "Search"},
                                 std::pair{res::icons::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    widgets::NavigationBar bar({items}, fonts());
    bar.set_width(360.0);
    return bar;
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

TEST_CASE("without labels the navigation bar fades them and shortens to 48 dp with the pill centered") {
    auto bar = make();
    bar.set_labels(false);
    double last = bar.height();
    int between = 0;
    for (int i = 0; i < 480; ++i) {
        bar.step(1.0 / 240.0);
        CHECK(std::abs(bar.height() - last) < 2.0);
        between += bar.height() > 49.0 && bar.height() < 63.0 ? 1 : 0;
        last = bar.height();
    }
    CHECK(between >= 6);
    settle(bar);
    CHECK(bar.height() == doctest::Approx(48.0));
    const render::Rect pill = bar.indicator_rect(0);
    CHECK(pill.y == doctest::Approx(8.0));
    CHECK(pill.height == doctest::Approx(32.0));
    CHECK(click(bar, 180.0, 24.0));
    CHECK(bar.selected() == 1);
    bar.set_labels(true);
    bar.snap_to_targets();
    CHECK(bar.height() == doctest::Approx(64.0));
    CHECK(bar.indicator_rect(0).y == doctest::Approx(6.0));
}
