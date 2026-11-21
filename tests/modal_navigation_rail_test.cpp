#include <doctest/doctest.h>

#include <vector>

#include "lamina/widgets/modal_navigation_rail.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;

namespace {

constexpr render::Rect area{0.0, 0.0, 900.0, 600.0};
constexpr const tokens::NavigationRailTokens& rail_t = tokens::navigation_rail_tokens;

widgets::ModalNavigationRailOptions options(bool hide) {
    const icons::Icon home{icon_font(), test::res::icons::home};
    const icons::Icon star{icon_font(), test::res::icons::star};
    return {{{{home, "Home"}, {star, "Starred"}, {star, "Saved"}}, 0}, hide};
}

void place(widgets::ModalNavigationRail& r) {
    r.set_area(area);
    const layout::Size s = r.measure({0.0, area.width, 0.0, area.height});
    r.arrange({area.x, area.y, s.width, s.height});
}

}

TEST_CASE("a modal wide rail keeps its collapsed width in the layout and expands over a scrim") {
    widgets::ModalNavigationRail r(options(false), fonts());
    place(r);
    CHECK(r.measure({0.0, area.width, 0.0, area.height}).width == doctest::Approx(rail_t.collapsed_width));
    CHECK_FALSE(r.showing());
    r.expand(input::InputSource::keyboard);
    settle(r);
    place(r);
    CHECK(r.showing());
    CHECK(r.bounds().width >= rail_t.expanded_min_width);
    CHECK(r.measure({0.0, area.width, 0.0, area.height}).width == doctest::Approx(rail_t.collapsed_width));
    CHECK(r.scrim_alpha() == doctest::Approx(1.0));
    CHECK(r.focused());
    const auto esc = r.handle(input::KeyEvent{Key::escape, false});
    CHECK(esc.collapsed);
    settle(r);
    CHECK_FALSE(r.showing());
    CHECK(r.bounds().width == doctest::Approx(rail_t.collapsed_width));
}

TEST_CASE("a click on the scrim collapses the modal rail and a click on an item selects it") {
    widgets::ModalNavigationRail r(options(false), fonts());
    place(r);
    r.expand();
    settle(r);
    const render::Rect item = r.rail().item_rect(1);
    r.handle(input::PointerEvent{PointerAction::down, item.x + 20.0, item.y + item.height / 2.0});
    const auto s = r.handle(input::PointerEvent{PointerAction::up, item.x + 20.0, item.y + item.height / 2.0});
    CHECK(s.selected);
    CHECK(r.rail().selected() == 1u);
    r.handle(input::PointerEvent{PointerAction::down, 700.0, 300.0});
    const auto c = r.handle(input::PointerEvent{PointerAction::up, 700.0, 300.0});
    CHECK(c.collapsed);
}

TEST_CASE("a rail that hides on collapse slides in from off screen and can be dragged away") {
    widgets::ModalNavigationRail r(options(true), fonts());
    place(r);
    CHECK(r.measure({0.0, area.width, 0.0, area.height}).width == doctest::Approx(0.0));
    CHECK_FALSE(r.showing());
    CHECK_FALSE(r.accessible());
    r.expand();
    settle(r);
    CHECK(r.offset() == doctest::Approx(0.0));
    CHECK(r.bounds().x == doctest::Approx(0.0));
    const double w = r.bounds().width;
    CHECK(w >= rail_t.expanded_min_width);
    double now = 0.0;
    r.handle(input::PointerEvent{PointerAction::down, 100.0, 500.0, 0, input::PointerKind::mouse, now});
    for (int i = 1; i <= 30; ++i) {
        now += frame_seconds;
        r.handle(input::PointerEvent{PointerAction::move, 100.0 - i * 5.0, 500.0, 0, input::PointerKind::mouse, now});
    }
    const auto u = r.handle(input::PointerEvent{PointerAction::up, -50.0, 500.0, 0, input::PointerKind::mouse, now});
    CHECK(u.collapsed);
    settle(r);
    CHECK_FALSE(r.showing());
    const auto node = r.accessible();
    CHECK_FALSE(node);
}

TEST_CASE("the expanded modal rail reports itself as modal with a close rail target") {
    widgets::ModalNavigationRail r(options(false), fonts());
    place(r);
    r.expand();
    settle(r);
    const auto node = r.accessible();
    REQUIRE(node);
    CHECK(node->state.modal);
    CHECK(widgets::find_accessible(node->children, "Close rail"));
    CHECK(widgets::find_accessible(node->children, "Starred"));
}
