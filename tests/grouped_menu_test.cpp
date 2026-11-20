#include <doctest/doctest.h>

#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using widgets::MenuSelection;

namespace {

icons::Icon icon(icons::Symbol s) {
    return {icon_font(), s};
}

widgets::GroupedMenu make() {
    widgets::MenuGroup view{{{"List", icon(res::icons::view_list), std::nullopt, std::nullopt, true},
                             {"Grid", icon(res::icons::grid_view)}},
                            "View",
                            true,
                            MenuSelection::single};
    widgets::MenuGroup options{{{"Bookmarked", icon(res::icons::bookmark), std::nullopt, icon(res::icons::check)},
                                {"Starred", icon(res::icons::star)},
                                {"Hidden", icon(res::icons::visibility_off)}},
                               {},
                               false,
                               MenuSelection::multiple};
    widgets::MenuGroup actions{{{"Share", icon(res::icons::share)}}};
    return widgets::GroupedMenu({{view, options, actions}}, fonts());
}

constexpr layout::Size window{600.0, 700.0};
constexpr render::Rect anchor{0.0, 40.0, 40.0, 40.0};

render::Point center(const render::Rect& r) {
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

widgets::GroupedMenuResponse click(widgets::GroupedMenu& m, const render::Rect& r) {
    const render::Point c = center(r);
    m.handle({PointerAction::down, c.x, c.y});
    return m.handle({PointerAction::up, c.x, c.y});
}

}

TEST_CASE("grouped menu tokens follow StandardMenuTokens.kt, SegmentedMenuTokens.kt, MenuDefaults.kt and Menu.kt") {
    const auto& t = tokens::grouped_menu_tokens;
    CHECK(t.group_container == color::Role::surface_container_low);
    CHECK(tokens::elevation_dp[t.elevation] == 3.0);
    CHECK(t.group_gap == 2.0);
    CHECK(t.group_vertical_padding == 2.0);
    CHECK(t.item_min_height == 44.0);
    CHECK(t.item_horizontal_inset == 4.0);
    CHECK(t.leading_icon_size == 20.0);
    CHECK(t.icon_text_space == 8.0);
    CHECK(t.selected_item == tokens::CornerSize::medium);
    CHECK(t.inactive_group == tokens::CornerSize::small);
    CHECK(t.label == tokens::TypeRole::label_large);
    CHECK(t.selected_container == color::Role::tertiary_container);
    CHECK(t.selected_label == color::Role::on_tertiary_container);
    CHECK(t.window_horizontal_margin == 8.0);
}

TEST_CASE("group and item corners depend on their position") {
    using tokens::CornerSize;
    CHECK(widgets::menu_group_corners(0, 1).top_start == CornerSize::large);
    CHECK(widgets::menu_group_corners(0, 1).bottom_end == CornerSize::large);
    CHECK(widgets::menu_group_corners(0, 3).bottom_start == CornerSize::small);
    CHECK(widgets::menu_group_corners(1, 3).top_start == CornerSize::small);
    CHECK(widgets::menu_group_corners(2, 3).bottom_end == CornerSize::large);
    CHECK(widgets::menu_item_corners(0, 1).top_start == CornerSize::extra_small);
    CHECK(widgets::menu_item_corners(0, 2).top_end == CornerSize::medium);
    CHECK(widgets::menu_item_corners(0, 2).bottom_end == CornerSize::extra_small);
    CHECK(widgets::menu_item_corners(1, 2).bottom_start == CornerSize::medium);
}

TEST_CASE("groups share one width, are 2 dp apart and hold 44 dp items inset by 4 dp") {
    auto m = make();
    m.open(anchor, window);
    CHECK(m.group_count() == 3);
    const render::Rect g0 = m.group_rect(0);
    const render::Rect g1 = m.group_rect(1);
    CHECK(g0.width == m.width());
    CHECK(g1.width == m.width());
    CHECK(g1.y == doctest::Approx(g0.y + g0.height + 2.0));
    CHECK(g0.height == doctest::Approx(2.0 + 32.0 + 5.0 + 2.0 * 44.0 + 2.0));
    CHECK(g1.height == doctest::Approx(2.0 + 3.0 * 44.0 + 2.0));
    const render::Rect item = m.item_rect(1, 0);
    CHECK(item.x == doctest::Approx(g1.x + 4.0));
    CHECK(item.width == doctest::Approx(m.width() - 8.0));
    CHECK(item.y == doctest::Approx(g1.y + 2.0));
    CHECK(item.height == 44.0);
    CHECK(m.width() >= 112.0 + 8.0);
    CHECK(m.height() == doctest::Approx(m.group_rect(2).y + m.group_rect(2).height - m.bounds().y));
}

TEST_CASE("the grouped menu keeps 8 dp from the window's side edges") {
    auto m = make();
    m.open(anchor, window);
    CHECK(m.bounds().x == doctest::Approx(8.0));
    CHECK(m.bounds().y == doctest::Approx(80.0));
}

TEST_CASE("a selected item morphs to 12 dp corners on the fast spatial spring") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    CHECK(m.item_radii(0, 0).bottom_left == doctest::Approx(12.0));
    CHECK(m.item_radii(0, 1).top_left == doctest::Approx(4.0));
    CHECK(m.item_radii(0, 1).bottom_left == doctest::Approx(12.0));
    const auto response = click(m, m.item_rect(0, 1));
    REQUIRE(response.chosen);
    CHECK(response.chosen->group == 0);
    CHECK(response.chosen->item == 1);
    CHECK(m.is_open());
    CHECK(m.selected(0, 1));
    CHECK_FALSE(m.selected(0, 0));
    double peak = 0.0;
    for (int i = 0; i < 120; ++i) {
        m.step(1.0 / 240.0);
        peak = std::max(peak, m.item_radii(0, 1).top_left);
    }
    CHECK(peak > 12.0);
    settle(m);
    CHECK(m.item_radii(0, 1).top_left == doctest::Approx(12.0));
    CHECK(m.item_radii(0, 0).bottom_left == doctest::Approx(4.0));
}

TEST_CASE("multiple selection toggles, and a plain group closes the menu") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    click(m, m.item_rect(1, 1));
    click(m, m.item_rect(1, 2));
    CHECK(m.selected(1, 1));
    CHECK(m.selected(1, 2));
    click(m, m.item_rect(1, 1));
    CHECK_FALSE(m.selected(1, 1));
    CHECK(m.is_open());
    const auto response = click(m, m.item_rect(2, 0));
    REQUIRE(response.chosen);
    CHECK(response.chosen->group == 2);
    CHECK_FALSE(m.is_open());
}

TEST_CASE("a group that has been hovered and is left takes the 8 dp inactive shape") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    CHECK(m.group_radii(0).top_left == doctest::Approx(16.0));
    CHECK(m.group_radii(0).bottom_left == doctest::Approx(8.0));
    const render::Point in0 = center(m.item_rect(0, 0));
    const render::Point in1 = center(m.item_rect(1, 0));
    m.handle({PointerAction::move, in0.x, in0.y});
    settle(m);
    CHECK(m.group_radii(0).top_left == doctest::Approx(16.0));
    m.handle({PointerAction::move, in1.x, in1.y});
    settle(m);
    CHECK(m.group_radii(0).top_left == doctest::Approx(8.0));
    CHECK(m.group_radii(2).bottom_left == doctest::Approx(16.0));
    m.handle({PointerAction::move, in0.x, in0.y});
    settle(m);
    CHECK(m.group_radii(0).top_left == doctest::Approx(16.0));
    CHECK(m.handle({PointerAction::down, 590.0, 690.0}).consumed);
    CHECK_FALSE(m.is_open());
}

TEST_CASE("the grouped menu paints groups on surface-container-low and the selection on tertiary-container") {
    auto m = make();
    m.open(anchor, window);
    settle(m);
    thorvg::Canvas canvas(600, 700);
    canvas.clear(0xffffffff);
    m.paint(canvas, scheme());
    const render::Rect selected = m.item_rect(0, 0);
    const render::Rect plain = m.item_rect(0, 1);
    const int sx = static_cast<int>(selected.x + selected.width - 30.0);
    CHECK(canvas.pixel(sx, static_cast<int>(selected.y + 8.0)) == scheme()[color::Role::tertiary_container]);
    CHECK(canvas.pixel(sx, static_cast<int>(plain.y + 8.0)) == scheme()[color::Role::surface_container_low]);
    const render::Rect gap{m.bounds().x, m.group_rect(0).y + m.group_rect(0).height, m.width(), 2.0};
    CHECK(canvas.pixel(static_cast<int>(gap.x + gap.width / 2.0), static_cast<int>(gap.y + 1.0)) !=
          scheme()[color::Role::surface_container_low]);
}
