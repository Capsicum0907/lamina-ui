#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/side_sheet.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::SideSheetValue;

namespace {

constexpr const tokens::SideSheetTokens& t = tokens::side_sheet_tokens;
constexpr render::Rect area{0.0, 0.0, 900.0, 600.0};

struct Content {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    widgets::Button* button = nullptr;
    layout::NodePtr node;

    Content() {
        button = &widgets.emplace<widgets::Button>(widgets::ButtonOptions{"Apply filter"}, fonts());
        focus.add(*button);
        node = layout::column({widgets::node(*button)});
    }
    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

widgets::SideSheetOptions options(Content& c, widgets::SideSheetKind kind = widgets::SideSheetKind::modal, bool detached = false) {
    return {.kind = kind,
            .detached = detached,
            .headline = "Filters",
            .close = widgets::SideSheetIcon{{icon_font(), test::res::icons::close}, "Close"},
            .back = widgets::SideSheetIcon{{icon_font(), test::res::icons::home}, "Back"},
            .content = c.slot(),
            .actions = {widgets::ButtonOptions{"Save"}, widgets::ButtonOptions{"Cancel", tokens::ButtonStyle::outlined}}};
}

struct Clock {
    double now = 0.0;
    input::PointerEvent at(PointerAction a, double x, double y) { return {a, x, y, 0, input::PointerKind::mouse, now}; }
};

widgets::SideSheetResponse drag(widgets::SideSheet& s, Clock& clock, double y, double from, double to, double seconds) {
    s.handle(clock.at(PointerAction::down, from, y));
    const int n = std::max(2, static_cast<int>(seconds / frame_seconds));
    for (int i = 1; i <= n; ++i) {
        clock.now += frame_seconds;
        s.handle(clock.at(PointerAction::move, from + (to - from) * i / n, y));
        s.step(frame_seconds);
    }
    return s.handle(clock.at(PointerAction::up, to, y));
}

}

TEST_CASE("a modal side sheet slides in from the end at 256 dp over a scrim in 275 ms") {
    Content c;
    widgets::SideSheet s(options(c), fonts());
    s.set_area(area);
    CHECK_FALSE(s.visible());
    s.open();
    for (int i = 0; i < 8; ++i) {
        s.step(frame_seconds);
    }
    CHECK(s.progress() > 0.0);
    CHECK(s.progress() < 1.0);
    for (int i = 0; i < 12; ++i) {
        s.step(frame_seconds);
    }
    CHECK(s.current_value() == SideSheetValue::expanded);
    const render::Rect b = s.bounds();
    CHECK(b.width == doctest::Approx(t.width));
    CHECK(b.x == doctest::Approx(area.width - t.width));
    CHECK(b.height == doctest::Approx(area.height));
    CHECK(s.progress() == doctest::Approx(1.0));
}

TEST_CASE("a detached side sheet keeps 16 dp from the window edges and a start sheet sits at the start") {
    Content c;
    widgets::SideSheet s(options(c, widgets::SideSheetKind::modal, true), fonts());
    s.set_area(area);
    s.snap_to(SideSheetValue::expanded);
    CHECK(s.bounds().x == doctest::Approx(area.width - t.detached_margin - t.width));
    CHECK(s.bounds().y == doctest::Approx(t.detached_margin));
    CHECK(s.bounds().height == doctest::Approx(area.height - 2.0 * t.detached_margin));
    Content d;
    widgets::SideSheetOptions o = options(d);
    o.edge = widgets::SideSheetEdge::start;
    widgets::SideSheet start(std::move(o), fonts());
    start.set_area(area);
    start.snap_to(SideSheetValue::expanded);
    CHECK(start.bounds().x == doctest::Approx(0.0));
}

TEST_CASE("Escape, the scrim and the close button dismiss a modal side sheet") {
    Content c;
    widgets::SideSheet s(options(c), fonts());
    s.set_area(area);
    s.snap_to(SideSheetValue::expanded);
    CHECK(s.handle(input::KeyEvent{Key::escape, false}).dismissed);
    settle(s);
    CHECK_FALSE(s.visible());
    s.snap_to(SideSheetValue::expanded);
    Clock clock;
    s.handle(clock.at(PointerAction::down, 100.0, 300.0));
    CHECK(s.handle(clock.at(PointerAction::up, 100.0, 300.0)).dismissed);
    s.snap_to(SideSheetValue::expanded);
    const render::Rect close = s.close_button()->bounds();
    s.handle(clock.at(PointerAction::down, close.x + 10.0, close.y + 10.0));
    CHECK(s.handle(clock.at(PointerAction::up, close.x + 10.0, close.y + 10.0)).dismissed);
    s.snap_to(SideSheetValue::expanded);
    const render::Rect back = s.back_button()->bounds();
    s.handle(clock.at(PointerAction::down, back.x + 10.0, back.y + 10.0));
    const auto r = s.handle(clock.at(PointerAction::up, back.x + 10.0, back.y + 10.0));
    CHECK(r.back);
    CHECK_FALSE(r.dismissed);
}

TEST_CASE("dragging a side sheet past half way or flinging it outwards hides it") {
    Content c;
    widgets::SideSheet s(options(c), fonts());
    s.set_area(area);
    s.snap_to(SideSheetValue::expanded);
    Clock clock;
    const double x = area.width - t.width + 40.0;
    CHECK_FALSE(drag(s, clock, 300.0, x, x + 60.0, 1.0).dismissed);
    settle(s);
    CHECK(s.current_value() == SideSheetValue::expanded);
    CHECK(drag(s, clock, 300.0, x, x + 160.0, 1.0).dismissed);
    settle(s);
    CHECK(s.current_value() == SideSheetValue::hidden);
    s.snap_to(SideSheetValue::expanded);
    CHECK(drag(s, clock, 300.0, x, x + 40.0, 0.05).dismissed);
}

TEST_CASE("a standard side sheet takes room in the layout as it opens and lets events beside it through") {
    Content c;
    widgets::SideSheet s(options(c, widgets::SideSheetKind::standard), fonts());
    CHECK(s.measure({0.0, 900.0, 0.0, 600.0}).width == doctest::Approx(0.0));
    s.arrange({600.0, 0.0, 0.0, 600.0});
    s.open();
    settle(s);
    CHECK(s.measure({0.0, 900.0, 0.0, 600.0}).width == doctest::Approx(t.width));
    s.arrange({644.0, 0.0, t.width, 600.0});
    CHECK(s.bounds().x == doctest::Approx(644.0));
    Clock clock;
    CHECK_FALSE(s.handle(clock.at(PointerAction::down, 100.0, 300.0)).consumed);
    CHECK_FALSE(s.handle(input::KeyEvent{Key::escape, false}).dismissed);
}

TEST_CASE("a modal side sheet opened from the keyboard keeps Tab among its buttons and content") {
    Content c;
    widgets::SideSheet s(options(c), fonts());
    s.set_area(area);
    s.open(input::InputSource::keyboard);
    settle(s);
    CHECK(s.back_button()->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.close_button()->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(c.button->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.action_button(0)->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.action_button(1)->focused());
    const auto r = s.handle(input::KeyEvent{Key::enter, false});
    CHECK(r.action == 1u);
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.back_button()->focused());
}

TEST_CASE("the side sheet reports a modal dialog with its headline, buttons, content and scrim") {
    Content c;
    widgets::SideSheet s(options(c), fonts());
    s.set_area(area);
    s.snap_to(SideSheetValue::expanded);
    const auto node = s.accessible();
    REQUIRE(node);
    CHECK(node->role == widgets::AccessibleRole::dialog);
    CHECK(node->name == "Filters");
    CHECK(node->state.modal);
    CHECK(widgets::find_accessible(node->children, "Close"));
    CHECK(widgets::find_accessible(node->children, "Apply filter"));
    CHECK(widgets::find_accessible(node->children, "Save"));
    CHECK(widgets::find_accessible(node->children, "Close sheet"));
}
