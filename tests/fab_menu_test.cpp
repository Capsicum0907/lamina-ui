#include <doctest/doctest.h>

#include <algorithm>

#include "lamina/widgets/extended_fab.hpp"
#include "lamina/widgets/fab_menu.hpp"
#include "lamina/widgets/protocol.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::ExtendedFab;
using widgets::FabMenu;
using widgets::ToggleFab;

namespace {

icons::Icon icon(icons::Symbol s) {
    return {icon_font(), s};
}

widgets::ToggleFabOptions fab_options(tokens::FabSize size = tokens::FabSize::baseline) {
    return {icon(res::icons::add), icon(res::icons::close), "Menu", size};
}

FabMenu menu() {
    FabMenu m({{{"Share", icon(res::icons::share)}, {"Edit", icon(res::icons::edit)}, {"Delete", icon(res::icons::delete_)}}, fab_options()},
              fonts());
    m.set_origin({0.0, 0.0});
    return m;
}

void click_at(FabMenu& m, render::Rect r) {
    m.handle({PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
    m.handle({PointerAction::up, r.x + r.width / 2.0, r.y + r.height / 2.0});
    m.handle({PointerAction::leave, 0.0, 0.0});
}

}

static_assert(widgets::Sized<ExtendedFab>);
static_assert(widgets::Focusable<ExtendedFab>);
static_assert(widgets::Sized<ToggleFab>);
static_assert(widgets::Sized<FabMenu>);

TEST_CASE("extended FAB sizes follow ExtendedFab{Small,Medium,Large}Tokens.kt and FloatingActionButton.kt") {
    const auto& s = tokens::extended_fab_size(tokens::ExtendedFabSize::small);
    CHECK(s.height == 56.0);
    CHECK(s.corner == tokens::CornerSize::large);
    CHECK(s.icon_label_space == 8.0);
    const auto& m = tokens::extended_fab_size(tokens::ExtendedFabSize::medium);
    CHECK(m.height == 80.0);
    CHECK(m.corner == tokens::CornerSize::large_increased);
    CHECK(m.icon_label_space == 12.0);
    CHECK(m.label == tokens::TypeRole::title_large);
    const auto& l = tokens::extended_fab_size(tokens::ExtendedFabSize::large);
    CHECK(l.height == 96.0);
    CHECK(l.icon_label_space == 16.0);
    CHECK(l.label == tokens::TypeRole::headline_small);
}

TEST_CASE("an extended FAB collapses to a square and fades its label") {
    ExtendedFab f({"Compose", icon(res::icons::edit), tokens::ExtendedFabSize::medium}, fonts());
    CHECK(f.width() == doctest::Approx(f.expanded_width()));
    CHECK(f.width() > f.height());
    f.set_expanded(false);
    CHECK(f.animating());
    settle(f);
    CHECK(f.width() == doctest::Approx(80.0));
    CHECK(f.label_alpha() == doctest::Approx(0.0));
    f.set_expanded(true);
    settle(f);
    CHECK(f.width() == doctest::Approx(f.expanded_width()));
    CHECK(f.accessible().name == "Compose");
}

TEST_CASE("toggle FAB tokens follow FloatingActionButtonMenu.kt and FabMenuBaselineTokens.kt") {
    const auto& t = tokens::toggle_fab_tokens;
    CHECK(t.initial_corner[0] == 16.0);
    CHECK(t.initial_corner[1] == 20.0);
    CHECK(t.initial_corner[2] == 28.0);
    CHECK(t.final_size == 56.0);
    CHECK(t.final_icon_size == 20.0);
    const auto& m = tokens::fab_menu_tokens;
    CHECK(m.item_height == 56.0);
    CHECK(m.item_between_space == 4.0);
    CHECK(m.close_button_between_space == 8.0);
    CHECK(m.stagger.stiffness == tokens::motion_scheme.slow_effects().stiffness);
}

TEST_CASE("checking a toggle FAB rounds it into a 56 dp circle anchored at its top end and shrinks the icon") {
    ToggleFab f(fab_options(tokens::FabSize::medium));
    f.set_origin({0.0, 0.0});
    CHECK(f.container().width == 80.0);
    CHECK(f.corner_radius() == 20.0);
    f.set_checked(true);
    settle(f);
    CHECK(f.container().width == doctest::Approx(56.0));
    CHECK(f.container().x == doctest::Approx(24.0));
    CHECK(f.container().y == doctest::Approx(0.0));
    CHECK(f.corner_radius() == doctest::Approx(28.0));
    CHECK(f.icon_size() == doctest::Approx(20.0));
    CHECK(f.accessible().state.toggle == widgets::AccessibleToggle::on);
}

TEST_CASE("opening a FAB menu reveals its items one by one from the bottom") {
    auto m = menu();
    CHECK(m.visible_count() == 0);
    m.set_expanded(true);
    std::vector<int> first_frame(3, -1);
    for (int f = 0; f < settle_frames; ++f) {
        m.step(frame_seconds);
        for (std::size_t i = 0; i < 3; ++i) {
            if (first_frame[i] < 0 && m.item_alpha(i) > 0.0) {
                first_frame[i] = f;
            }
        }
    }
    CHECK(m.visible_count() == 3);
    CHECK(first_frame[2] <= first_frame[1]);
    CHECK(first_frame[1] <= first_frame[0]);
    CHECK(first_frame[2] < first_frame[0]);
    CHECK(m.item_width_progress(0) == doctest::Approx(1.0));
}

TEST_CASE("the items stack above the button with 4 dp between and 8 dp over the button") {
    auto m = menu();
    m.set_expanded(true);
    settle(m);
    const render::Rect last = m.item_rect(2);
    const render::Rect prev = m.item_rect(1);
    CHECK(last.y - (prev.y + prev.height) == doctest::Approx(4.0));
    CHECK(m.button().bounds().y - (last.y + last.height) == doctest::Approx(8.0));
    CHECK(m.bounds().y + m.bounds().height - (m.button().bounds().y + m.button().height()) == doctest::Approx(16.0));
    CHECK(last.x + last.width == doctest::Approx(m.button().bounds().x + m.button().width()));
}

TEST_CASE("choosing an item reports it and closes the menu, and closing hides the top item first") {
    auto m = menu();
    m.set_expanded(true);
    settle(m);
    click_at(m, m.item_rect(1));
    CHECK_FALSE(m.expanded());
    std::vector<int> gone(3, -1);
    for (int f = 0; f < settle_frames; ++f) {
        m.step(frame_seconds);
        for (std::size_t i = 0; i < 3; ++i) {
            if (gone[i] < 0 && m.item_width_progress(i) < 0.99) {
                gone[i] = f;
            }
        }
    }
    CHECK(gone[0] <= gone[2]);
    CHECK(m.visible_count() == 0);
}

TEST_CASE("the button toggles the menu and the keyboard moves into the items") {
    auto m = menu();
    click_at(m, m.button().container());
    CHECK(m.expanded());
    settle(m);
    m.button().set_focused(true);
    CHECK(m.handle(input::KeyEvent{Key::down}).consumed);
    CHECK(m.focused_item() == 0);
    const auto r = m.handle(input::KeyEvent{Key::enter});
    REQUIRE(r.chosen);
    CHECK(*r.chosen == 0);
    CHECK_FALSE(m.expanded());
    CHECK(m.button().focused());
}

TEST_CASE("Escape closes an open menu and the items are focus stops only while shown") {
    auto m = menu();
    auto stops = m.focus_stops();
    REQUIRE(stops.size() == 4);
    CHECK_FALSE(stops[1]->can_focus());
    m.set_expanded(true);
    settle(m);
    CHECK(stops[1]->can_focus());
    CHECK(m.handle(input::KeyEvent{Key::escape}).consumed);
    CHECK_FALSE(m.expanded());
    CHECK_FALSE(stops[1]->can_focus());
}

TEST_CASE("the menu's accessible node lists the shown items and the toggle button") {
    auto m = menu();
    CHECK(m.accessible().children.size() == 1);
    m.set_expanded(true);
    settle(m);
    const auto node = m.accessible();
    REQUIRE(node.children.size() == 4);
    CHECK(node.children[0].name == "Share");
    CHECK(node.children.back().role == widgets::AccessibleRole::toggle_button);
}
