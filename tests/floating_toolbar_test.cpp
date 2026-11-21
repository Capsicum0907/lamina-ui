#include <doctest/doctest.h>

#include "lamina/widgets/flexible_bottom_app_bar.hpp"
#include "lamina/widgets/floating_toolbar.hpp"
#include "lamina/widgets/protocol.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::FloatingToolbar;
using widgets::FloatingToolbarOptions;
using widgets::ToolbarAction;
using widgets::ToolbarSlot;

namespace {

icons::Icon icon(icons::Symbol s) {
    return {icon_font(), s};
}

std::vector<ToolbarAction> actions() {
    return {{icon(res::icons::edit), "Edit"}, {icon(res::icons::share), "Share"}, {icon(res::icons::delete_), "Delete"}};
}

FloatingToolbar with_fab(widgets::ToolbarOrientation o = widgets::ToolbarOrientation::horizontal) {
    FloatingToolbarOptions opts{actions()};
    opts.fab = widgets::ToolbarFab{icon(res::icons::add), "New"};
    opts.orientation = o;
    opts.accessible_name = "Tools";
    FloatingToolbar t(std::move(opts), icon_font());
    t.set_origin({0.0, 0.0});
    return t;
}

}

static_assert(widgets::Sized<FloatingToolbar>);
static_assert(widgets::Disableable<FloatingToolbar>);
static_assert(widgets::WidthSettable<widgets::FlexibleBottomAppBar>);

TEST_CASE("floating toolbar tokens follow FloatingToolbarTokens.kt, DockedToolbarTokens.kt and FloatingToolbar.kt") {
    const auto& t = tokens::floating_toolbar_tokens;
    CHECK(t.container_size == 64.0);
    CHECK(t.content_padding == 8.0);
    CHECK(t.fab_min_size == 56.0);
    CHECK(t.fab_max_size == 80.0);
    CHECK(t.toolbar_to_fab_gap == 8.0);
    CHECK(t.scroll_threshold == 40.0);
    CHECK(t.standard.container == color::Role::surface_container);
    CHECK(t.vibrant.container == color::Role::primary_container);
    CHECK(t.vibrant.fab_container == color::Role::tertiary_container);
    const auto& b = tokens::flexible_bottom_app_bar_tokens;
    CHECK(b.height == 64.0);
    CHECK(b.leading_space == 16.0);
    CHECK(b.fixed_spacing == 32.0);
}

TEST_CASE("a toolbar is 64 dp tall with 48 dp slots inside 8 dp of padding") {
    FloatingToolbar t({actions()}, icon_font());
    CHECK(t.height() == 64.0);
    CHECK(t.width() == doctest::Approx(16.0 + 3.0 * 48.0));
    t.set_origin({0.0, 0.0});
    CHECK(t.action(ToolbarSlot::content, 1).bounds().x == doctest::Approx(8.0 + 48.0 + 4.0));
}

TEST_CASE("leading and trailing content shrink away when the toolbar collapses") {
    FloatingToolbarOptions o{actions()};
    o.leading = {{icon(res::icons::home), "Home"}};
    o.trailing = {{icon(res::icons::settings), "Settings"}};
    FloatingToolbar t(std::move(o), icon_font());
    const double open = t.width();
    CHECK(open == doctest::Approx(16.0 + 5.0 * 48.0));
    t.set_expanded(false);
    settle(t);
    CHECK(t.width() == doctest::Approx(16.0 + 3.0 * 48.0));
    CHECK(t.accessible().children.size() == 3);
    t.set_expanded(true);
    settle(t);
    CHECK(t.width() == doctest::Approx(open));
}

TEST_CASE("collapsing a toolbar with a FAB shrinks the bar into it while the FAB grows to 80 dp") {
    auto t = with_fab();
    const double full = 16.0 + 3.0 * 48.0;
    CHECK(t.width() == doctest::Approx(full + 8.0 + 56.0));
    CHECK(t.height() == 80.0);
    CHECK(t.fab_rect()->width == 56.0);
    CHECK(t.toolbar_rect().width == doctest::Approx(full));
    CHECK(t.toolbar_rect().y == doctest::Approx(8.0));
    t.set_expanded(false);
    settle(t);
    CHECK(t.toolbar_rect().width == doctest::Approx(0.0));
    CHECK(t.toolbar_rect().x == doctest::Approx(full));
    CHECK(t.fab_rect()->width == doctest::Approx(80.0));
    CHECK(t.fab_rect()->x + 80.0 == doctest::Approx(t.width()));
    CHECK(t.width() == doctest::Approx(full + 8.0 + 56.0));
}

TEST_CASE("a vertical toolbar stacks its actions and puts the FAB at the bottom") {
    auto t = with_fab(widgets::ToolbarOrientation::vertical);
    CHECK(t.width() == 80.0);
    CHECK(t.height() == doctest::Approx(16.0 + 3.0 * 48.0 + 8.0 + 56.0));
    CHECK(t.action(ToolbarSlot::content, 1).bounds().y > t.action(ToolbarSlot::content, 0).bounds().y);
    CHECK(t.fab_rect()->y + 56.0 == doctest::Approx(t.height()));
}

TEST_CASE("the toolbar reports clicks on its actions and its FAB") {
    auto t = with_fab();
    const render::Rect b = t.action(ToolbarSlot::content, 2).bounds();
    t.handle({PointerAction::down, b.x + 5.0, b.y + 5.0});
    const auto r = t.handle({PointerAction::up, b.x + 5.0, b.y + 5.0});
    REQUIRE(r.clicked);
    CHECK(r.clicked->index == 2);
    const render::Rect f = *t.fab_rect();
    t.handle({PointerAction::down, f.x + 10.0, f.y + 10.0});
    CHECK(t.handle({PointerAction::up, f.x + 10.0, f.y + 10.0}).fab_clicked);
}

TEST_CASE("collapsed toolbar actions are not focus stops but the FAB stays one") {
    auto t = with_fab();
    auto stops = t.focus_stops();
    REQUIRE(stops.size() == 4);
    CHECK(stops[0]->can_focus());
    t.set_expanded(false);
    CHECK_FALSE(stops[0]->can_focus());
    CHECK(stops[3]->can_focus());
    t.set_fab_focused(true);
    CHECK(t.handle(input::KeyEvent{Key::enter}).fab_clicked);
}

TEST_CASE("the toolbar exposes a toolbar role and offers to collapse or expand") {
    auto t = with_fab();
    auto node = t.accessible();
    CHECK(node.role == widgets::AccessibleRole::toolbar);
    CHECK(node.description == "Collapse toolbar");
    CHECK(node.children.size() == 4);
    t.set_expanded(false);
    CHECK(t.accessible().description == "Expand toolbar");
}

TEST_CASE("a disabled toolbar ignores its FAB") {
    auto t = with_fab();
    t.set_enabled(false);
    const render::Rect f = *t.fab_rect();
    t.handle({PointerAction::down, f.x + 10.0, f.y + 10.0});
    CHECK_FALSE(t.handle({PointerAction::up, f.x + 10.0, f.y + 10.0}).fab_clicked);
    CHECK_FALSE(t.action(ToolbarSlot::content, 0).enabled());
}

TEST_CASE("scrolling 40 dp toward the end collapses and scrolling 40 dp back expands") {
    widgets::ToolbarScrollExpansion s(true);
    CHECK_FALSE(s.scrolled_to(30.0));
    CHECK(s.scrolled_to(45.0) == false);
    CHECK_FALSE(s.scrolled_to(200.0));
    CHECK_FALSE(s.scrolled_to(170.0));
    CHECK(s.scrolled_to(150.0) == true);
    CHECK(s.expanded());
}

TEST_CASE("an exit-always bottom bar follows the scroll and settles on the nearer end") {
    widgets::FlexibleBottomAppBar bar({actions()});
    bar.set_width(400.0);
    bar.set_bottom(600.0);
    CHECK(bar.height() == 64.0);
    CHECK(bar.bounds().y == doctest::Approx(536.0));
    bar.state().scrolled_to(20.0);
    CHECK(bar.height() == doctest::Approx(44.0));
    bar.state().settle();
    settle(bar);
    CHECK(bar.height() == doctest::Approx(64.0));
    bar.state().scrolled_to(60.0);
    bar.state().settle();
    settle(bar);
    CHECK(bar.height() == doctest::Approx(0.0));
    CHECK(bar.accessible().children.empty());
    bar.state().scrolled_to(10.0);
    CHECK(bar.height() == doctest::Approx(50.0));
}

TEST_CASE("the bottom bar spreads its actions between 16 dp paddings or packs them 32 dp apart") {
    widgets::FlexibleBottomAppBar bar({actions()});
    bar.set_width(400.0);
    bar.set_origin({0.0, 0.0});
    CHECK(bar.action(0).bounds().x == doctest::Approx(16.0 + 4.0));
    CHECK(bar.action(2).bounds().x + bar.action(2).width() == doctest::Approx(400.0 - 16.0 - 4.0));
    widgets::FlexibleBottomAppBar fixed({actions(), widgets::BottomBarArrangement::fixed});
    fixed.set_width(400.0);
    fixed.set_origin({0.0, 0.0});
    CHECK(fixed.action(1).bounds().x - fixed.action(0).bounds().x == doctest::Approx(48.0 + 32.0));
}

TEST_CASE("pointer moves over a toolbar or its FAB are consumed so the content under it does not hover") {
    auto t = with_fab();
    const render::Rect bar = t.toolbar_rect();
    CHECK(t.handle({PointerAction::move, bar.x + 4.0, bar.y + bar.height / 2.0}).consumed);
    const render::Rect f = *t.fab_rect();
    CHECK(t.handle({PointerAction::move, f.x + 4.0, f.y + 4.0}).consumed);
    CHECK_FALSE(t.handle({PointerAction::move, 900.0, 900.0}).consumed);
}
