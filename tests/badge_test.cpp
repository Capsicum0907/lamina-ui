#include <doctest/doctest.h>

#include <string>

#include "lamina/widgets/badge.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::Badge;
using widgets::BadgeSpec;

TEST_CASE("badge tokens follow BadgeTokens and the BadgedBox offsets") {
    const auto& t = tokens::badge_tokens;
    CHECK(t.small_size == 6.0);
    CHECK(t.large_size == 16.0);
    CHECK(t.large_horizontal_padding == 4.0);
    CHECK(t.small_offset == 6.0);
    CHECK(t.large_horizontal_offset == 12.0);
    CHECK(t.large_vertical_offset == 14.0);
    CHECK(t.label == tokens::TypeRole::label_small);
    CHECK(t.container == color::Role::error);
    CHECK(t.content == color::Role::on_error);
}

TEST_CASE("a count above the maximum shows the maximum and a plus") {
    CHECK(widgets::badge_count(0) == "0");
    CHECK(widgets::badge_count(7) == "7");
    CHECK(widgets::badge_count(999) == "999");
    CHECK(widgets::badge_count(1000) == "999+");
    CHECK(widgets::badge_count(123456) == "999+");
    CHECK(widgets::badge_count(120, 99) == "99+");
    CHECK(widgets::badge_count(-3) == "0");
}

TEST_CASE("a small badge is a 6 dp dot inside the anchor's top end corner") {
    Badge dot({}, fonts());
    CHECK(dot.small());
    CHECK(dot.size().width == 6.0);
    const render::Rect r = dot.place({100.0, 50.0, 24.0, 24.0});
    CHECK(r.x == 118.0);
    CHECK(r.y == 50.0);
    CHECK(r.width == 6.0);
}

TEST_CASE("a large badge is at least 16 dp, starts 12 dp before the anchor's end and 2 dp above it") {
    Badge one({"3"}, fonts());
    CHECK_FALSE(one.small());
    CHECK(one.size().width == 16.0);
    CHECK(one.size().height == 16.0);
    const render::Rect r = one.place({100.0, 50.0, 24.0, 24.0});
    CHECK(r.x == 112.0);
    CHECK(r.y == 48.0);
    Badge many({"999+"}, fonts());
    CHECK(many.size().width > 16.0);
    CHECK(many.place({100.0, 50.0, 24.0, 24.0}).x == 112.0);
}

TEST_CASE("bounds keep the badge inside an item's end and top edges, as badgeBounds does") {
    Badge many({"999+"}, fonts());
    const render::Rect item{80.0, 50.0, 50.0, 64.0};
    const render::Rect r = many.place({100.0, 50.0, 24.0, 24.0}, item);
    CHECK(r.x + r.width == doctest::Approx(130.0));
    CHECK(r.y == 50.0);
}

TEST_CASE("a badge paints the error container and its text in on-error") {
    Badge one({"8"}, fonts());
    thorvg::Canvas canvas(64, 64);
    canvas.clear(0xffffffff);
    one.paint(canvas, scheme(), {20.0, 20.0, 24.0, 24.0});
    CHECK(canvas.pixel(34, 26) == scheme()[color::Role::error]);
    CHECK(count_color(canvas, scheme()[color::Role::on_error]) > 0);
}

TEST_CASE("an icon button carries a badge on its icon and names it in its description") {
    widgets::IconButton b({icons::Icon{icon_font(), res::icons::home}, "Inbox"});
    b.set_origin({0.0, 0.0});
    b.set_badge(Badge({"12", "12 new messages"}, fonts()));
    CHECK(b.accessible().description == "12 new messages");
    const render::Rect icon = b.icon_rect();
    CHECK(icon.width == 24.0);
    const render::Rect badge = b.badge()->place(icon);
    CHECK(badge.x == doctest::Approx(icon.x + 12.0));
    b.set_badge(std::nullopt);
    CHECK(b.accessible().description.empty());
}

TEST_CASE("navigation items carry badges on their icons, kept inside the item, and change their count") {
    std::vector<widgets::NavigationItem> items{{icons::Icon{icon_font(), res::icons::home}, "Home"},
                                               {icons::Icon{icon_font(), res::icons::send}, "Mail", {}, {}, true, {}, BadgeSpec{"3"}}};
    widgets::NavigationBar bar({items}, fonts());
    bar.set_width(200.0);
    CHECK_FALSE(bar.item_badge(0).has_value());
    REQUIRE(bar.item_badge(1).has_value());
    CHECK(bar.accessible().children[1].description == "3");
    bar.set_item_badge(1, BadgeSpec{widgets::badge_count(1200), "More than 999 new messages"});
    CHECK(bar.item_badge(1)->spec().text == "999+");
    CHECK(bar.accessible().children[1].description == "More than 999 new messages");
    bar.set_item_badge(0, BadgeSpec{});
    CHECK(bar.item_badge(0)->small());
    thorvg::Canvas canvas(200, 80);
    canvas.clear(0xffffffff);
    bar.paint(canvas, scheme());
    CHECK(count_color(canvas, scheme()[color::Role::error]) > 0);

    widgets::NavigationRail rail({items}, fonts());
    rail.set_item_badge(0, BadgeSpec{"5"});
    CHECK(rail.accessible().children[0].description == "5");
}
