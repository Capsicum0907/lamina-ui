#include <doctest/doctest.h>

#include <array>

#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/chip.hpp"
#include "lamina/widgets/chip_group.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;
using input::PointerKind;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using tokens::Density;

namespace {

constexpr double stack_gap = 8.0;
constexpr double narrow_group = 40.0;
constexpr double unbounded = 10000.0;

widgets::Button extra_small(double y) {
    widgets::Button b({"Send", ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round}, fonts());
    b.set_origin({10.0, y});
    return b;
}

bool tap(widgets::Button& b, double x, double y, PointerKind kind = PointerKind::touch) {
    b.handle({PointerAction::down, x, y, 1, kind});
    return b.handle({PointerAction::up, x, y, 0, kind});
}

struct Stack {
    widgets::WidgetSet set;
    std::array<int, 2> clicks{};
    widgets::Button* upper = nullptr;
    widgets::Button* lower = nullptr;

    Stack() {
        upper = &set.emplace_with<widgets::Button>([this](bool c) { clicks[0] += c ? 1 : 0; }, widgets::ButtonOptions{"One", ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round}, fonts());
        lower = &set.emplace_with<widgets::Button>([this](bool c) { clicks[1] += c ? 1 : 0; }, widgets::ButtonOptions{"Two", ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round}, fonts());
        upper->set_origin({10.0, 10.0});
        lower->set_origin({10.0, upper->bounds().y + upper->bounds().height + stack_gap});
    }

    void tap(double x, double y, PointerKind kind = PointerKind::touch) {
        set.handle({PointerAction::down, x, y, 1, kind});
        set.handle({PointerAction::up, x, y, 0, kind});
    }
};

}

TEST_CASE("target_hit tells a hit inside the bounds from one in the margin, and measures the margin from the edge") {
    const render::Rect r{10.0, 10.0, 60.0, 32.0};
    const auto inside = widgets::target_hit(r, 20.0, 20.0);
    REQUIRE(inside);
    CHECK(inside->inside);
    const auto above = widgets::target_hit(r, 20.0, 7.0);
    REQUIRE(above);
    CHECK_FALSE(above->inside);
    CHECK(above->distance_squared == doctest::Approx(9.0));
    CHECK_FALSE(widgets::target_hit(r, 20.0, 1.0));
    CHECK_FALSE(widgets::target_hit(r, 5.0, 20.0));
    CHECK(inside->beats(*above));
    CHECK_FALSE(above->beats(*inside));
}

TEST_CASE("hit_area keeps the margin until another receiver has claimed it") {
    const render::Rect r{10.0, 10.0, 60.0, 32.0};
    input::PointerEvent e{PointerAction::down, 20.0, 7.0};
    CHECK(widgets::hit_area(e, r).height == tokens::minimum_interactive_size);
    e.margin_claimed = true;
    CHECK(widgets::hit_area(e, r).height == r.height);
}

TEST_CASE("an extra-small button takes a tap up to the 48 dp touch target, centred on it, at the default density only") {
    auto b = extra_small(10.0);
    const render::Rect r = b.bounds();
    CHECK(r.height == 32.0);
    const double margin = (tokens::minimum_interactive_size - r.height) / 2.0;
    CHECK(tap(b, r.x + 5.0, r.y - margin + 0.5));
    CHECK(tap(b, r.x + 5.0, r.y + r.height + margin - 0.5));
    CHECK_FALSE(tap(b, r.x + 5.0, r.y - margin - 0.5));
    CHECK(b.bounds().height == 32.0);
    CHECK(b.height() == 32.0);
    b.set_density(Density{-1});
    const render::Rect dense = b.bounds();
    CHECK_FALSE(tap(b, dense.x + 5.0, dense.y - 1.0));
    CHECK(tap(b, dense.x + 5.0, dense.y + 1.0));
}

TEST_CASE("a mouse click in the margin of a small button counts too") {
    auto b = extra_small(10.0);
    const render::Rect r = b.bounds();
    CHECK(tap(b, r.x + 5.0, r.y - 3.0, PointerKind::mouse));
}

TEST_CASE("of two stacked buttons whose touch targets overlap, the nearer one takes a tap in the gap") {
    Stack s;
    const render::Rect up = s.upper->bounds();
    const render::Rect down = s.lower->bounds();
    REQUIRE(down.y - (up.y + up.height) == stack_gap);
    const double x = up.x + 5.0;
    s.tap(x, up.y + up.height + 2.0);
    CHECK(s.clicks == std::array<int, 2>{1, 0});
    s.tap(x, down.y - 2.0);
    CHECK(s.clicks == std::array<int, 2>{1, 1});
}

TEST_CASE("a tap inside a button's bounds wins over a neighbour's margin that reaches it") {
    Stack s;
    s.lower->set_origin({s.upper->bounds().x, s.upper->bounds().y + s.upper->bounds().height + 2.0});
    const render::Rect down = s.lower->bounds();
    s.tap(down.x + 5.0, down.y + 1.0);
    CHECK(s.clicks == std::array<int, 2>{0, 1});
}

TEST_CASE("the button that took the press keeps its margin until the release") {
    Stack s;
    const render::Rect up = s.upper->bounds();
    const render::Rect down = s.lower->bounds();
    const double x = up.x + 5.0;
    s.set.handle({PointerAction::down, x, up.y + up.height + 1.0, 1, PointerKind::touch});
    s.set.handle({PointerAction::move, x, down.y - 1.0, 0, PointerKind::touch});
    s.set.handle({PointerAction::up, x, up.y + up.height + 3.0, 0, PointerKind::touch});
    CHECK(s.clicks == std::array<int, 2>{1, 0});
}

TEST_CASE("a mouse press in two overlapping targets presses only the nearer button") {
    Stack s;
    const render::Rect up = s.upper->bounds();
    const double x = up.x + 5.0;
    const double y = up.y + up.height + 2.0;
    s.set.handle({PointerAction::down, x, y, 1, PointerKind::mouse});
    CHECK(s.upper->pressed());
    CHECK_FALSE(s.lower->pressed());
    s.set.handle({PointerAction::up, x, y, 0, PointerKind::mouse});
    CHECK(s.clicks == std::array<int, 2>{1, 0});
}

TEST_CASE("a set nested in a set competes with the best target of its members") {
    widgets::WidgetSet outer;
    int outer_clicks = 0;
    int inner_clicks = 0;
    auto& a = outer.emplace_with<widgets::Button>([&](bool c) { outer_clicks += c ? 1 : 0; }, widgets::ButtonOptions{"One", ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round}, fonts());
    auto& inner = outer.emplace<widgets::WidgetSet>();
    auto& b = inner.emplace_with<widgets::Button>([&](bool c) { inner_clicks += c ? 1 : 0; }, widgets::ButtonOptions{"Two", ButtonStyle::filled, ButtonSize::extra_small, ButtonShape::round}, fonts());
    a.set_origin({10.0, 10.0});
    b.set_origin({10.0, a.bounds().y + a.bounds().height + stack_gap});
    const double x = a.bounds().x + 5.0;
    const double near_b = b.bounds().y - 2.0;
    outer.handle({PointerAction::down, x, near_b, 1, PointerKind::touch});
    outer.handle({PointerAction::up, x, near_b, 0, PointerKind::touch});
    CHECK(outer_clicks == 0);
    CHECK(inner_clicks == 1);
    const double near_a = a.bounds().y + a.bounds().height + 2.0;
    outer.handle({PointerAction::down, x, near_a, 1, PointerKind::touch});
    outer.handle({PointerAction::up, x, near_a, 0, PointerKind::touch});
    CHECK(outer_clicks == 1);
    CHECK(inner_clicks == 1);
}

TEST_CASE("chips in a group share overlapping margins the same way") {
    widgets::ChipGroup group({.chips = {{.label = "One"}, {.label = "Two"}}}, fonts());
    const layout::Size size = group.measure({0.0, narrow_group, 0.0, unbounded});
    group.arrange({10.0, 10.0, size.width, size.height});
    const render::Rect first = group.chip(0).bounds();
    const render::Rect second = group.chip(1).bounds();
    REQUIRE(second.y > first.y + first.height);
    REQUIRE(second.y - (first.y + first.height) < tokens::minimum_interactive_size - first.height);
    const double x = first.x + 5.0;
    const double y = first.y + first.height + 1.0;
    group.handle({PointerAction::down, x, y, 1, PointerKind::touch});
    const widgets::ChipGroupResponse r = group.handle({PointerAction::up, x, y, 0, PointerKind::touch});
    REQUIRE(r.clicked);
    CHECK(*r.clicked == 0);
}
