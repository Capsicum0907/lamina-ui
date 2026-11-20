#include <doctest/doctest.h>

#include "lamina/motion/easing.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/top_app_bar.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using tokens::TopAppBarKind;
using widgets::TopAppBar;
using widgets::TopAppBarOptions;
using widgets::TopAppBarScroll;
using widgets::TopAppBarState;

namespace {

widgets::IconButtonOptions button(icons::Symbol s, const char* name) {
    return {icons::Icon{icon_font(), s}, name};
}

TopAppBar bar(TopAppBarKind kind, TopAppBarScroll scroll = TopAppBarScroll::pinned, bool icons = true) {
    TopAppBarOptions o{"Inbox", kind};
    o.scroll = scroll;
    if (icons) {
        o.navigation = button(res::icons::home, "Back");
        o.actions = {button(res::icons::search, "Search"), button(res::icons::star, "Star")};
    }
    TopAppBar b(o, fonts());
    b.arrange({0.0, 0.0, 400.0, b.height()});
    return b;
}

TopAppBarState state(TopAppBarScroll behavior, double limit) {
    TopAppBarState s(behavior);
    s.set_height_offset_limit(limit);
    return s;
}

}

TEST_CASE("top app bar tokens follow AppBar.kt and the app bar token files") {
    const auto& t = tokens::top_app_bar_tokens;
    CHECK(t.collapsed_height == 64.0);
    CHECK(t.horizontal_padding == 4.0);
    CHECK(t.title_inset == 12.0);
    CHECK(t.container == color::Role::surface);
    CHECK(t.scrolled_container == color::Role::surface_container);
    CHECK(t.action_icon == color::Role::on_surface_variant);
    CHECK(t.navigation_icon == color::Role::on_surface);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::small).expanded_height == 64.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::medium).expanded_height == 112.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::medium).title == tokens::TypeRole::headline_small);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::large).expanded_height == 152.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::large).title_bottom_padding == 28.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::medium_flexible).expanded_height_with_subtitle == 136.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::large_flexible).expanded_height == 120.0);
    CHECK(tokens::top_app_bar_kind(TopAppBarKind::large_flexible).title == tokens::TypeRole::display_small);
    CHECK(motion::ease(t.top_title_alpha, 0.5) < 0.2);
}

TEST_CASE("pinned keeps the bar and counts the content under it") {
    TopAppBarState s = state(TopAppBarScroll::pinned, -64.0);
    s.scrolled_to(40.0);
    CHECK(s.height_offset() == 0.0);
    CHECK(s.content_scroll() == 40.0);
    CHECK(s.overlapped_fraction() == doctest::Approx(40.0 / 64.0));
    CHECK_FALSE(s.needs_settle());
    s.scrolled_to(0.0);
    CHECK(s.overlapped_fraction() == 0.0);
}

TEST_CASE("enter-always hides the bar first and shows it again on any scroll back") {
    TopAppBarState s = state(TopAppBarScroll::enter_always, -64.0);
    s.scrolled_to(30.0);
    CHECK(s.height_offset() == -30.0);
    CHECK(s.content_scroll() == 0.0);
    s.scrolled_to(200.0);
    CHECK(s.height_offset() == -64.0);
    CHECK(s.content_scroll() == doctest::Approx(136.0));
    s.scrolled_to(180.0);
    CHECK(s.height_offset() == -44.0);
    CHECK(s.content_scroll() == doctest::Approx(136.0));
    CHECK(s.scroll_offset() == doctest::Approx(180.0));
    CHECK(s.collapsed_fraction() == doctest::Approx(44.0 / 64.0));
}

TEST_CASE("exit-until-collapsed collapses first and expands only once the content is back at the top") {
    TopAppBarState s = state(TopAppBarScroll::exit_until_collapsed, 64.0 - 152.0);
    s.scrolled_to(40.0);
    CHECK(s.height_offset() == -40.0);
    CHECK(s.content_scroll() == 0.0);
    s.scrolled_to(300.0);
    CHECK(s.height_offset() == -88.0);
    CHECK(s.content_scroll() == doctest::Approx(212.0));
    CHECK(s.collapsed_fraction() == doctest::Approx(1.0));
    s.scrolled_to(200.0);
    CHECK(s.height_offset() == -88.0);
    CHECK(s.content_scroll() == doctest::Approx(112.0));
    s.scrolled_to(60.0);
    CHECK(s.content_scroll() == 0.0);
    CHECK(s.height_offset() == -60.0);
    s.scrolled_to(0.0);
    CHECK(s.height_offset() == 0.0);
}

TEST_CASE("a partly collapsed bar settles to the nearer end on the snap spring, keeping the content where it is") {
    TopAppBarState s = state(TopAppBarScroll::exit_until_collapsed, -88.0);
    s.scrolled_to(30.0);
    CHECK(s.needs_settle());
    s.settle();
    CHECK(s.settling());
    for (int i = 0; i < 120; ++i) {
        s.step(frame_seconds);
    }
    CHECK_FALSE(s.settling());
    CHECK(s.height_offset() == 0.0);
    CHECK(s.scroll_offset() == 0.0);
    s.scrolled_to(60.0);
    s.settle();
    for (int i = 0; i < 120; ++i) {
        s.step(frame_seconds);
    }
    CHECK(s.height_offset() == -88.0);
    CHECK(s.scroll_offset() == doctest::Approx(88.0));
    CHECK(s.content_scroll() == 0.0);
    TopAppBarState tiny = state(TopAppBarScroll::enter_always, -64.0);
    tiny.scrolled_to(0.5);
    CHECK_FALSE(tiny.needs_settle());
}

TEST_CASE("a small bar is 64 dp, the title starts 16 dp in, or 56 dp after a navigation icon") {
    TopAppBar plain = bar(TopAppBarKind::small, TopAppBarScroll::pinned, false);
    CHECK(plain.height() == 64.0);
    CHECK(plain.title_rect().x == doctest::Approx(16.0));
    TopAppBar icons = bar(TopAppBarKind::small);
    CHECK(icons.title_rect().x == doctest::Approx(56.0));
    CHECK(icons.navigation_button()->bounds().x == doctest::Approx(8.0));
    CHECK(icons.action(1).bounds().x + icons.action(1).bounds().width == doctest::Approx(400.0 - 4.0 - 4.0));
    CHECK(icons.action(0).bounds().y == doctest::Approx(12.0));
}

TEST_CASE("a center-aligned title is centered on the bar and pushed in only when it would meet the icons") {
    TopAppBar b = bar(TopAppBarKind::center_aligned);
    const render::Rect r = b.title_rect();
    CHECK(r.x + r.width / 2.0 == doctest::Approx(200.0));
}

TEST_CASE("medium and large bars put the title in a bottom row, fading it as the top title fades in") {
    TopAppBar large = bar(TopAppBarKind::large, TopAppBarScroll::exit_until_collapsed);
    CHECK(large.height() == 152.0);
    CHECK(large.title_rect().x == doctest::Approx(16.0));
    CHECK(large.title_opacity() == 1.0);
    CHECK(large.collapsed_title_opacity() == 0.0);
    const render::Rect title = large.title_rect();
    CHECK(title.y + title.height <= 152.0);
    CHECK(title.y > 64.0);
    widgets::ScrollArea scroll;
    scroll.set_viewport({0.0, 0.0, 400.0, 600.0});
    scroll.set_content_height(152.0 + 2000.0);
    scroll.jump_to(44.0);
    large.follow(scroll);
    CHECK(large.height() == doctest::Approx(108.0));
    CHECK(large.title_opacity() == doctest::Approx(0.5));
    CHECK(large.collapsed_title_opacity() == doctest::Approx(motion::ease(tokens::top_app_bar_tokens.top_title_alpha, 0.5)));
    CHECK(large.container_amount() == doctest::Approx(motion::ease(tokens::easing::legacy_accelerate, 0.5)));
    scroll.jump_to(500.0);
    large.follow(scroll);
    CHECK(large.height() == 64.0);
    CHECK(large.collapsed_title_opacity() == doctest::Approx(1.0));
    CHECK(large.title_opacity() == doctest::Approx(0.0));
}

TEST_CASE("following a scroll area settles a half-collapsed bar once the scroll comes to rest") {
    TopAppBar medium = bar(TopAppBarKind::medium, TopAppBarScroll::exit_until_collapsed);
    widgets::ScrollArea scroll;
    scroll.set_viewport({0.0, 0.0, 400.0, 600.0});
    scroll.set_content_height(112.0 + 2000.0);
    scroll.scroll_by(30.0);
    for (int i = 0; i < 240; ++i) {
        scroll.step(frame_seconds);
        medium.step(frame_seconds);
        medium.follow(scroll);
    }
    CHECK(medium.height() == doctest::Approx(64.0));
    CHECK(scroll.offset() == doctest::Approx(48.0));
    CHECK(medium.state().content_scroll() == doctest::Approx(0.0));
}

TEST_CASE("a small bar takes the scrolled container on the effects spring once content is under it") {
    TopAppBar small = bar(TopAppBarKind::small);
    widgets::ScrollArea scroll;
    scroll.set_viewport({0.0, 0.0, 400.0, 600.0});
    scroll.set_content_height(64.0 + 2000.0);
    scroll.jump_to(10.0);
    small.follow(scroll);
    CHECK(small.container_amount() == 0.0);
    small.step(frame_seconds);
    CHECK(small.container_amount() > 0.0);
    settle(small);
    CHECK(small.container_amount() == doctest::Approx(1.0));
    thorvg::Canvas canvas(400, 64);
    canvas.clear(0xffffffff);
    small.paint(canvas, scheme());
    CHECK(canvas.pixel(200, 2) == scheme()[color::Role::surface_container]);
}

TEST_CASE("an enter-always small bar shrinks away and comes back") {
    TopAppBar small = bar(TopAppBarKind::small, TopAppBarScroll::enter_always);
    widgets::ScrollArea scroll;
    scroll.set_viewport({0.0, 0.0, 400.0, 600.0});
    scroll.set_content_height(64.0 + 2000.0);
    scroll.jump_to(100.0);
    small.follow(scroll);
    CHECK(small.height() == 0.0);
    scroll.jump_to(80.0);
    small.follow(scroll);
    CHECK(small.height() == doctest::Approx(20.0));
}

TEST_CASE("the bar's buttons click, it takes the pointer over it, and it names its parts") {
    TopAppBar b = bar(TopAppBarKind::small);
    const render::Rect nav = b.navigation_button()->bounds();
    b.handle({input::PointerAction::down, nav.x + 20.0, nav.y + 20.0});
    CHECK(b.handle({input::PointerAction::up, nav.x + 20.0, nav.y + 20.0}).navigation);
    const render::Rect star = b.action(1).bounds();
    b.handle({input::PointerAction::down, star.x + 20.0, star.y + 20.0});
    CHECK(b.handle({input::PointerAction::up, star.x + 20.0, star.y + 20.0}).action == 1u);
    CHECK(b.handle({input::PointerAction::move, 200.0, 30.0}).consumed);
    CHECK_FALSE(b.handle({input::PointerAction::move, 200.0, 90.0}).consumed);
    const widgets::AccessibleNode node = b.accessible();
    CHECK(node.role == widgets::AccessibleRole::toolbar);
    REQUIRE(node.children.size() == 4);
    CHECK(node.children[1].name == "Inbox");
    CHECK(node.children[2].name == "Search");
    b.set_enabled(false);
    CHECK_FALSE(b.action(0).enabled());
}

TEST_CASE("a flexible bar takes a subtitle and is taller with it") {
    TopAppBarOptions o{"Photos", TopAppBarKind::medium_flexible, "1,024 items"};
    TopAppBar b(o, fonts());
    CHECK(b.expanded_height() == 136.0);
    TopAppBarOptions m{"Photos", TopAppBarKind::medium, "dropped"};
    TopAppBar medium(m, fonts());
    CHECK(medium.expanded_height() == 112.0);
    CHECK(medium.accessible().children[0].description.empty());
}
