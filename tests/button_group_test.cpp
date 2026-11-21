#include <doctest/doctest.h>

#include <algorithm>
#include <limits>

#include "lamina/widgets/button_group.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/protocol.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::ButtonGroup;
using widgets::ButtonGroupItem;
using widgets::ButtonGroupOptions;
using widgets::ButtonGroupSelection;
using tokens::CornerSize;

namespace {

constexpr double unbounded = std::numeric_limits<double>::infinity();

ButtonGroup make(ButtonGroupOptions o, double max_width = unbounded) {
    o.accessible_name = "Group";
    ButtonGroup g(std::move(o), fonts(), icon_font());
    const layout::Size s = g.measure({0.0, max_width, 0.0, unbounded});
    g.arrange({0.0, 0.0, s.width, s.height});
    g.set_window({800.0, 600.0});
    return g;
}

std::vector<ButtonGroupItem> five() {
    return {{"Alpha"}, {"Bravo"}, {"Charlie"}, {"Delta"}, {"Echo"}};
}

render::Point centre(const widgets::Button& b) {
    const render::Rect r = b.bounds();
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

render::Point centre(const widgets::IconButton& b) {
    const render::Rect r = b.bounds();
    return {r.x + r.width / 2.0, r.y + r.height / 2.0};
}

template <typename W>
void click_on(ButtonGroup& g, const W& w) {
    const render::Point p = centre(w);
    g.handle({PointerAction::down, p.x, p.y});
    g.handle({PointerAction::up, p.x, p.y});
    g.handle({PointerAction::leave, 0.0, 0.0});
}

double total_width(const ButtonGroup& g) {
    double w = 0.0;
    for (std::size_t i = 0; i < g.visible_count(); ++i) {
        w += g.item_width(i);
    }
    return w;
}

}

static_assert(widgets::Measurable<ButtonGroup>);
static_assert(widgets::Disableable<ButtonGroup>);

TEST_CASE("button group tokens follow ButtonGroupSmallTokens.kt, ConnectedButtonGroupSmallTokens.kt and ButtonGroup.kt") {
    const auto& t = tokens::button_group_tokens;
    CHECK(t.between_space == 12.0);
    CHECK(t.connected_between_space == 2.0);
    CHECK(t.expanded_ratio == doctest::Approx(0.15));
    CHECK(t.release_threshold == doctest::Approx(0.75));
    CHECK(t.connected_inner_corner == CornerSize::small);
    CHECK(t.connected_pressed_inner_corner == CornerSize::extra_small);
    CHECK(t.width_spring.stiffness == tokens::motion_scheme.fast_spatial().stiffness);
}

TEST_CASE("a button group lays its buttons out at their own widths with 12 dp between") {
    auto g = make({five()});
    double expected = 4.0 * 12.0;
    for (std::size_t i = 0; i < g.size(); ++i) {
        expected += g.button(i).intrinsic_width();
    }
    CHECK(g.bounds().width == doctest::Approx(expected));
    CHECK(g.button(1).bounds().x == doctest::Approx(g.button(0).bounds().width + 12.0));
    CHECK_FALSE(g.overflowing());
}

TEST_CASE("pressing a middle button grows it into both neighbours by its ratio, capped by their end padding") {
    auto g = make({five()});
    const double w0 = g.item_width(1);
    const double w1 = g.item_width(2);
    const double w2 = g.item_width(3);
    const double total = total_width(g);
    const render::Point p = centre(g.button(2));
    g.handle({PointerAction::down, p.x, p.y});
    settle(g);
    const double growth = std::min(0.15 * w1 / 2.0, tokens::button_size(tokens::ButtonSize::small).trailing_space);
    CHECK(g.item_width(2) == doctest::Approx(w1 + 2.0 * growth));
    CHECK(g.item_width(1) == doctest::Approx(w0 - growth));
    CHECK(g.item_width(3) == doctest::Approx(w2 - growth));
    CHECK(total_width(g) == doctest::Approx(total));
    g.handle({PointerAction::up, p.x, p.y});
    settle(g);
    CHECK(g.item_width(2) == doctest::Approx(w1));
}

TEST_CASE("pressing the first button grows it only into the second by its full ratio") {
    auto g = make({five()});
    const double w0 = g.item_width(0);
    const double w1 = g.item_width(1);
    const render::Point p = centre(g.button(0));
    g.handle({PointerAction::down, p.x, p.y});
    settle(g);
    const double growth = std::min(0.15 * w0, tokens::button_size(tokens::ButtonSize::small).trailing_space);
    CHECK(g.item_width(0) == doctest::Approx(w0 + growth));
    CHECK(g.item_width(1) == doctest::Approx(w1 - growth));
}

TEST_CASE("a quick click still squeezes the neighbours: the release waits until the press passes 0.75") {
    auto g = make({five()});
    const render::Point p = centre(g.button(2));
    g.handle({PointerAction::down, p.x, p.y});
    g.handle({PointerAction::up, p.x, p.y});
    double peak = 0.0;
    for (int i = 0; i < settle_frames; ++i) {
        g.step(frame_seconds);
        peak = std::max(peak, g.press_progress(2));
    }
    CHECK(peak > 0.75);
    CHECK(g.press_progress(2) == doctest::Approx(0.0));
    CHECK_FALSE(g.animating());
}

TEST_CASE("animate_width off keeps the widths when a button is pressed") {
    ButtonGroupOptions o{five()};
    o.animate_width = false;
    auto g = make(std::move(o));
    const double w = g.item_width(2);
    const render::Point p = centre(g.button(2));
    g.handle({PointerAction::down, p.x, p.y});
    settle(g);
    CHECK(g.item_width(2) == doctest::Approx(w));
}

TEST_CASE("a group that is too narrow moves the trailing items into a More options menu") {
    auto full = make({five()});
    const double narrow = full.bounds().width * 0.6;
    auto g = make({five()}, narrow);
    CHECK(g.overflowing());
    CHECK(g.visible_count() < g.size());
    CHECK(g.bounds().width <= narrow);
    CHECK(g.overflow_button().accessible().name == "More options");
    double shown = 0.0;
    for (std::size_t i = 0; i < g.visible_count(); ++i) {
        shown += g.button(i).bounds().width + 12.0;
    }
    CHECK(g.overflow_button().bounds().x == doctest::Approx(shown));
    click_on(g, g.overflow_button());
    REQUIRE(g.menu_open());
    CHECK(g.menu()->count() == g.size() - g.visible_count());
    const render::Rect item = g.menu()->item_rect(0);
    g.handle({PointerAction::down, item.x + 10.0, item.y + item.height / 2.0});
    const auto r = g.handle({PointerAction::up, item.x + 10.0, item.y + item.height / 2.0});
    REQUIRE(r.clicked);
    CHECK(*r.clicked == g.visible_count());
    CHECK_FALSE(g.menu_open());
}

TEST_CASE("clicking the overflow button again closes its menu") {
    auto full = make({five()});
    auto g = make({five()}, full.bounds().width * 0.6);
    click_on(g, g.overflow_button());
    REQUIRE(g.menu_open());
    click_on(g, g.overflow_button());
    CHECK_FALSE(g.menu_open());
}

TEST_CASE("hidden items are not focus stops and the overflow button is one only while it shows") {
    auto wide = make({five()});
    auto stops = wide.focus_stops();
    REQUIRE(stops.size() == 6);
    CHECK(stops[4]->can_focus());
    CHECK_FALSE(stops[5]->can_focus());
    auto g = make({five()}, wide.bounds().width * 0.6);
    auto narrow_stops = g.focus_stops();
    CHECK_FALSE(narrow_stops[4]->can_focus());
    CHECK(narrow_stops[5]->can_focus());
}

TEST_CASE("Enter on the focused overflow button opens the menu for the keyboard, and Escape gives the focus back") {
    auto full = make({five()});
    auto g = make({five()}, full.bounds().width * 0.6);
    g.overflow_button().set_focused(true);
    CHECK(g.handle(input::KeyEvent{Key::enter}).consumed);
    REQUIRE(g.menu_open());
    CHECK(g.menu()->focused_item() == 0);
    CHECK(g.handle(input::KeyEvent{Key::escape}).consumed);
    CHECK_FALSE(g.menu_open());
    CHECK(g.overflow_button().focused());
}

TEST_CASE("a connected group sits 2 dp apart with full outer and small inner corners") {
    ButtonGroupOptions o{{{"Day"}, {"Week"}, {"Month"}}, ButtonGroupSelection::single, true};
    auto g = make(std::move(o));
    CHECK(g.button(1).bounds().x == doctest::Approx(g.button(0).bounds().width + 2.0));
    const auto lead = g.button(0).corner_radii();
    CHECK(lead.top_left == doctest::Approx(g.button(0).height() / 2.0));
    CHECK(lead.top_right == doctest::Approx(8.0));
    const auto mid = g.button(1).corner_radii();
    CHECK(mid.top_left == doctest::Approx(8.0));
    CHECK(mid.bottom_right == doctest::Approx(8.0));
    const auto trail = g.button(2).corner_radii();
    CHECK(trail.top_left == doctest::Approx(8.0));
    CHECK(trail.bottom_right == doctest::Approx(g.button(2).height() / 2.0));
}

TEST_CASE("a pressed connected button narrows its inner corners and a checked one turns into a pill") {
    ButtonGroupOptions o{{{"Day"}, {"Week"}, {"Month"}}, ButtonGroupSelection::multiple, true};
    auto g = make(std::move(o));
    const render::Point p = centre(g.button(1));
    g.handle({PointerAction::down, p.x, p.y});
    settle(g);
    CHECK(g.button(1).corner_radii().top_left == doctest::Approx(4.0));
    g.handle({PointerAction::up, p.x, p.y});
    settle(g);
    CHECK(g.checked(1));
    CHECK(g.button(1).corner_radii().top_left == doctest::Approx(g.button(1).height() / 2.0));
}

TEST_CASE("a single-select group keeps one radio checked") {
    ButtonGroupOptions o{{{"Day", std::nullopt, true}, {"Week", std::nullopt, true}, {"Month"}}, ButtonGroupSelection::single, true};
    auto g = make(std::move(o));
    CHECK(g.selected() == 0);
    CHECK_FALSE(g.checked(1));
    CHECK(g.button(2).accessible().role == widgets::AccessibleRole::radio_button);
    click_on(g, g.button(2));
    CHECK(g.selected() == 2);
    CHECK_FALSE(g.checked(0));
    click_on(g, g.button(2));
    CHECK(g.checked(2));
}

TEST_CASE("a multiple-select group toggles each button on its own") {
    ButtonGroupOptions o{{{"Bold"}, {"Italic"}, {"Underline"}}, ButtonGroupSelection::multiple};
    auto g = make(std::move(o));
    click_on(g, g.button(0));
    click_on(g, g.button(2));
    CHECK(g.checked(0));
    CHECK(g.checked(2));
    click_on(g, g.button(0));
    CHECK_FALSE(g.checked(0));
    CHECK(g.button(1).accessible().role == widgets::AccessibleRole::toggle_button);
}

TEST_CASE("a disabled group ignores presses and does not squeeze") {
    auto g = make({five()});
    g.set_enabled(false);
    const double w = g.item_width(2);
    const render::Point p = centre(g.button(2));
    g.handle({PointerAction::down, p.x, p.y});
    settle(g);
    CHECK(g.item_width(2) == doctest::Approx(w));
    CHECK_FALSE(g.handle({PointerAction::up, p.x, p.y}).clicked);
    CHECK_FALSE(g.accessible().state.enabled);
}

TEST_CASE("the group's accessible node lists its visible buttons and the overflow button") {
    auto full = make({five()});
    CHECK(full.accessible().role == widgets::AccessibleRole::group);
    CHECK(full.accessible().children.size() == 5);
    auto g = make({five()}, full.bounds().width * 0.6);
    CHECK(g.accessible().children.size() == g.visible_count() + 1);
    CHECK(g.accessible().children.back().name == "More options");
}

TEST_CASE("weighted items share the room left over") {
    ButtonGroupOptions o{{{"A", std::nullopt, false, true, {}, 1.0}, {"B", std::nullopt, false, true, {}, 3.0}}};
    auto g = make(std::move(o), 412.0);
    CHECK(g.item_width(0) == doctest::Approx(100.0));
    CHECK(g.item_width(1) == doctest::Approx(300.0));
}
