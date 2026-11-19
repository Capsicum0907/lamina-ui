#include <doctest/doctest.h>

#include <algorithm>
#include <optional>

#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

namespace {

widgets::NavigationRailOptions options() {
    std::vector<widgets::NavigationItem> items;
    for (auto [symbol, label] : {std::pair{icons::Symbol::home, "Home"}, std::pair{icons::Symbol::search, "Search"},
                                 std::pair{icons::Symbol::settings, "Settings"}}) {
        items.push_back({icons::Icon{icon_font(), symbol}, label});
    }
    return {items};
}

widgets::NavigationRail make(widgets::NavigationRailOptions o = options()) {
    widgets::NavigationRail rail(std::move(o), fonts());
    rail.measure({0.0, 800.0});
    rail.arrange({0.0, 0.0, rail.width(), 600.0});
    return rail;
}

}

TEST_CASE("navigation rail tokens follow NavigationRail*Tokens.kt and WideNavigationRail.kt") {
    const auto& t = tokens::navigation_rail_tokens;
    CHECK(t.collapsed_width == 96.0);
    CHECK(t.expanded_min_width == 220.0);
    CHECK(t.expanded_max_width == 360.0);
    CHECK(t.top_space == 44.0);
    CHECK(t.header_space == 40.0);
    CHECK(t.collapsed_item_space == 4.0);
    CHECK(t.collapsed_item_min_height == 64.0);
    CHECK(t.expanded_item_min_height == 48.0);
    CHECK(t.item_horizontal_padding == 20.0);
    CHECK(t.top_indicator_width == 56.0);
    CHECK(t.top_indicator_height == 32.0);
    CHECK(t.start_indicator_height == 56.0);
    CHECK(t.start_icon_label_space == 8.0);
    CHECK(t.top_label == tokens::TypeRole::label_medium);
    CHECK(t.start_label == tokens::TypeRole::label_large);
    CHECK(t.container == color::Role::surface);
    CHECK(t.indicator == color::Role::secondary_container);
    CHECK(t.top_active_label == color::Role::secondary);
    CHECK(t.start_active_label == color::Role::on_secondary_container);
    CHECK(t.state_layer == color::Role::on_surface);
}

TEST_CASE("the collapsed rail is 96 dp wide with 64 dp items whose pill sits 6 dp from the top") {
    auto rail = make();
    CHECK(rail.width() == 96.0);
    const render::Rect item = rail.item_rect(0);
    CHECK(item.y == doctest::Approx(44.0));
    CHECK(item.width == doctest::Approx(96.0));
    CHECK(item.height == doctest::Approx(64.0));
    CHECK(rail.item_rect(1).y == doctest::Approx(44.0 + 64.0 + 4.0));
    const render::Rect pill = rail.indicator_rect(0);
    CHECK(pill.x == doctest::Approx(20.0));
    CHECK(pill.y == doctest::Approx(50.0));
    CHECK(pill.width == doctest::Approx(56.0));
    CHECK(pill.height == doctest::Approx(32.0));
    CHECK(rail.indicator_rect(1).width == doctest::Approx(0.0));
    const render::Rect icon = rail.icon_rect(0);
    CHECK(icon.x == doctest::Approx(36.0));
    CHECK(icon.y == doctest::Approx(54.0));
    const render::Rect label = rail.label_rect(0);
    CHECK(label.x + label.width / 2.0 == doctest::Approx(48.0));
    CHECK(label.y == doctest::Approx(50.0 + 32.0 + 4.0));
    CHECK(label.height == doctest::Approx(16.0));
    CHECK(rail.label_opacity(0) == doctest::Approx(1.0));
}

TEST_CASE("a header pushes the items down by its height and 40 dp") {
    render::Rect placed{};
    widgets::NavigationRailOptions o = options();
    o.header = layout::fixed({72.0, 48.0}, [&placed](const render::Rect& r) { placed = r; });
    auto rail = make(std::move(o));
    CHECK(placed.x == doctest::Approx(0.0));
    CHECK(placed.y == doctest::Approx(44.0));
    CHECK(rail.item_rect(0).y == doctest::Approx(44.0 + 48.0 + 40.0));
}

TEST_CASE("selecting an item grows its pill from the start edge on the spatial spring") {
    auto rail = make();
    const render::Rect second = rail.item_rect(1);
    CHECK(click(rail, 48.0, second.y + 10.0));
    CHECK(rail.selected() == 1);
    rail.step(0.05);
    const render::Rect growing = rail.indicator_rect(1);
    CHECK(growing.x == doctest::Approx(20.0));
    CHECK(growing.width > 0.0);
    CHECK(growing.width < 56.0);
    settle(rail);
    CHECK(rail.indicator_rect(1).width == doctest::Approx(56.0));
    CHECK(rail.indicator_rect(0).width == doctest::Approx(0.0));
    CHECK_FALSE(click(rail, 48.0, second.y + 10.0));
}

TEST_CASE("expanding the rail springs its width to 220 dp and moves the labels beside the icons") {
    auto rail = make();
    rail.set_expanded(true);
    double peak = 0.0;
    double faintest = 1.0;
    for (int i = 0; i < 480; ++i) {
        rail.step(1.0 / 240.0);
        peak = std::max(peak, rail.width());
        faintest = std::min(faintest, rail.label_opacity(0));
    }
    CHECK(peak > 220.0);
    CHECK(faintest < 0.05);
    settle(rail);
    CHECK(rail.width() == doctest::Approx(220.0));
    const render::Rect item = rail.item_rect(0);
    CHECK(item.y == doctest::Approx(44.0));
    CHECK(item.height == doctest::Approx(56.0));
    CHECK(rail.item_rect(1).y == doctest::Approx(44.0 + 56.0));
    const render::Rect label = rail.label_rect(0);
    const render::Rect pill = rail.indicator_rect(0);
    CHECK(pill.x == doctest::Approx(20.0));
    CHECK(pill.height == doctest::Approx(56.0));
    CHECK(pill.width == doctest::Approx(16.0 + 24.0 + 8.0 + label.width + 16.0));
    CHECK(item.width == doctest::Approx(20.0 + pill.width));
    CHECK(rail.icon_rect(0).x == doctest::Approx(36.0));
    CHECK(rail.icon_rect(0).y == doctest::Approx(44.0 + 16.0));
    CHECK(label.x == doctest::Approx(20.0 + 16.0 + 24.0 + 8.0));
    CHECK(label.height == doctest::Approx(20.0));
    CHECK(label.y == doctest::Approx(44.0 + 18.0));
    CHECK(rail.label_opacity(0) == doctest::Approx(1.0));

    rail.set_expanded(false);
    settle(rail);
    CHECK(rail.width() == doctest::Approx(96.0));
    CHECK(rail.item_rect(0).height == doctest::Approx(64.0));
}

TEST_CASE("a copied rail keeps its layout after the original is gone") {
    std::optional<widgets::NavigationRail> original(make());
    original->set_expanded(true);
    settle(*original);
    const render::Rect label = original->label_rect(1);
    const render::Rect item = original->item_rect(2);
    widgets::NavigationRail copy = *original;
    original.reset();
    CHECK(copy.label_rect(1).width == label.width);
    CHECK(copy.label_rect(1).y == label.y);
    CHECK(copy.item_rect(2).y == item.y);
}

TEST_CASE("the rail is a layout node whose width follows its animation") {
    auto rail = make();
    render::Rect content{};
    auto filler = layout::fixed({0.0, 0.0}, [&content](const render::Rect& r) { content = r; });
    auto root = layout::row({widgets::node(rail), layout::weighted(filler, 1.0)});
    layout::run(*root, {0.0, 0.0, 800.0, 600.0});
    CHECK(rail.height() == doctest::Approx(600.0));
    CHECK(content.x == doctest::Approx(96.0));
    rail.set_expanded(true);
    settle(rail);
    layout::run(*root, {0.0, 0.0, 800.0, 600.0});
    CHECK(content.x == doctest::Approx(220.0));
}

TEST_CASE("the rail paints the selected pill on the surface") {
    auto rail = make();
    thorvg::Canvas canvas(120, 200);
    canvas.clear(0xffffffff);
    rail.paint(canvas, scheme());
    const render::Rect pill = rail.indicator_rect(0);
    CHECK(canvas.pixel(static_cast<int>(pill.x + 4.0), static_cast<int>(pill.y + 16.0)) == scheme()[color::Role::secondary_container]);
    CHECK(canvas.pixel(10, 66) == scheme()[color::Role::surface]);
    CHECK(canvas.pixel(100, 66) == 0xffffffff);
}
