#include <doctest/doctest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "lamina/widgets/bottom_sheet.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::SheetValue;

namespace {

constexpr render::Rect area{0.0, 0.0, 400.0, 800.0};
constexpr const tokens::BottomSheetTokens& t = tokens::bottom_sheet_tokens;

struct Content {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    std::vector<widgets::Button*> buttons;
    std::vector<int> clicks;
    layout::NodePtr node;

    Content(int count, double gap) {
        std::vector<layout::NodePtr> nodes;
        clicks.assign(static_cast<std::size_t>(count), 0);
        for (int i = 0; i < count; ++i) {
            auto& b = widgets.emplace_with<widgets::Button>(
                [this, i](widgets::Button&, bool clicked) {
                    if (clicked) {
                        ++clicks[static_cast<std::size_t>(i)];
                    }
                },
                widgets::ButtonOptions{"Item " + std::to_string(i)}, fonts());
            focus.add(b);
            buttons.push_back(&b);
            nodes.push_back(widgets::node(b));
        }
        auto c = layout::column(std::move(nodes));
        c->spacing(gap);
        node = c;
    }

    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

struct Clock {
    double now = 0.0;
    input::PointerEvent at(PointerAction a, double x, double y) { return {a, x, y, 0, input::PointerKind::mouse, now}; }
};

template <typename W>
void run(W& w, Clock& clock, double seconds = 3.0) {
    for (double s = 0.0; s < seconds; s += frame_seconds) {
        clock.now += frame_seconds;
        w.step(frame_seconds);
    }
}

void drag(widgets::BottomSheet& s, Clock& clock, double x, double from, double to, double seconds, bool release = true) {
    s.handle(clock.at(PointerAction::down, x, from));
    const int n = std::max(2, static_cast<int>(seconds / frame_seconds));
    for (int i = 1; i <= n; ++i) {
        clock.now += frame_seconds;
        s.handle(clock.at(PointerAction::move, x, from + (to - from) * i / n));
        s.step(frame_seconds);
    }
    if (release) {
        s.handle(clock.at(PointerAction::up, x, to));
    }
}

}

TEST_CASE("a modal sheet opens to half its height and fades the scrim in") {
    Content c(3, 8.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    CHECK_FALSE(s.visible());
    Clock clock;
    s.show();
    CHECK(s.target_value() == SheetValue::partially_expanded);
    run(s, clock);
    const double h = s.sheet_height();
    CHECK(h < area.height / 2.0);
    CHECK(s.current_value() == SheetValue::partially_expanded);
    CHECK(s.offset() == doctest::Approx(area.height - h / 2.0));
    CHECK(s.scrim_alpha() == doctest::Approx(1.0));
    CHECK(s.bounds().width == doctest::Approx(area.width));
}

TEST_CASE("a tall sheet opens to half the area and keeps the 640 dp width limit") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area({0.0, 0.0, 1200.0, 800.0});
    Clock clock;
    s.show();
    run(s, clock);
    CHECK(s.sheet_height() == doctest::Approx(800.0));
    CHECK(s.offset() == doctest::Approx(400.0));
    CHECK(s.bounds().width == doctest::Approx(t.max_width));
    CHECK(s.bounds().x == doctest::Approx((1200.0 - t.max_width) / 2.0));
    CHECK(s.content().scrolls());
}

TEST_CASE("dragging a sheet up fast expands it, and a short slow drag down springs back") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    drag(s, clock, 30.0, 500.0, 300.0, 0.15);
    CHECK(s.target_value() == SheetValue::expanded);
    run(s, clock);
    CHECK(s.current_value() == SheetValue::expanded);
    CHECK(s.offset() == doctest::Approx(0.0));
    drag(s, clock, 30.0, 300.0, 300.0 + t.positional_threshold - 10.0, 1.0);
    run(s, clock);
    CHECK(s.current_value() == SheetValue::expanded);
    drag(s, clock, 30.0, 300.0, 300.0 + t.positional_threshold + 30.0, 1.0);
    run(s, clock);
    CHECK(s.current_value() == SheetValue::partially_expanded);
}

TEST_CASE("an expanding spring overshoots the top and the container stretches instead of leaving a gap") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.snap_to(SheetValue::partially_expanded);
    s.expand();
    double lowest = area.height;
    for (int i = 0; i < 120; ++i) {
        s.step(frame_seconds);
        lowest = std::min(lowest, s.offset());
    }
    CHECK(lowest < 0.0);
    thorvg::Canvas canvas(400, 800);
    s.snap_to(SheetValue::partially_expanded);
    s.expand();
    for (int i = 0; i < 120 && s.offset() >= -1.0; ++i) {
        s.step(frame_seconds);
    }
    REQUIRE(s.offset() < -1.0);
    canvas.clear(0xff000000);
    s.paint(canvas, scheme());
    canvas.finish();
    CHECK(canvas.pixel(200, 799) == scheme()[t.container]);
}

TEST_CASE("flinging a sheet down dismisses it") {
    Content c(3, 8.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    const double top = s.bounds().y;
    s.handle(clock.at(PointerAction::down, 200.0, top + 10.0));
    widgets::BottomSheetResponse r;
    for (int i = 1; i <= 6; ++i) {
        clock.now += frame_seconds;
        s.handle(clock.at(PointerAction::move, 200.0, top + 10.0 + i * 12.0));
    }
    r = s.handle(clock.at(PointerAction::up, 200.0, top + 82.0));
    CHECK(r.dismissed);
    CHECK(s.target_value() == SheetValue::hidden);
    run(s, clock);
    CHECK_FALSE(s.visible());
}

TEST_CASE("Escape steps an expanded sheet down to half before it dismisses") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.snap_to(SheetValue::expanded);
    auto r = s.handle(input::KeyEvent{Key::escape, false});
    CHECK_FALSE(r.dismissed);
    CHECK(s.target_value() == SheetValue::partially_expanded);
    run(s, clock);
    r = s.handle(input::KeyEvent{Key::escape, false});
    CHECK(r.dismissed);
    CHECK(s.target_value() == SheetValue::hidden);
}

TEST_CASE("a click on the scrim dismisses the sheet, and a click on its content does not") {
    Content c(3, 8.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    const render::Rect b = c.buttons[0]->bounds();
    const double bx = s.content().viewport().x + b.x + b.width / 2.0;
    const double by = s.content().viewport().y + b.y + b.height / 2.0 - s.content().scroll().offset();
    s.handle(clock.at(PointerAction::down, bx, by));
    const auto pressed = s.handle(clock.at(PointerAction::up, bx, by));
    CHECK_FALSE(pressed.dismissed);
    CHECK(c.clicks[0] == 1);
    s.handle(clock.at(PointerAction::down, 200.0, 100.0));
    const auto r = s.handle(clock.at(PointerAction::up, 200.0, 100.0));
    CHECK(r.dismissed);
    CHECK(s.target_value() == SheetValue::hidden);
}

TEST_CASE("a drag that starts on a button moves the sheet and does not click the button") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    const render::Rect b = c.buttons[0]->bounds();
    const double bx = s.content().viewport().x + b.x + b.width / 2.0;
    const double by = s.content().viewport().y + b.y + b.height / 2.0;
    drag(s, clock, bx, by, by - 200.0, 0.2);
    CHECK(c.clicks[0] == 0);
    CHECK(s.target_value() == SheetValue::expanded);
}

TEST_CASE("the drag handle expands a half-open sheet and dismisses an expanded one") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    REQUIRE(s.drag_handle());
    const render::Rect h = s.drag_handle()->bounds();
    CHECK(h.width == doctest::Approx(t.drag_handle_width));
    CHECK(h.height == doctest::Approx(t.drag_handle_height + 2.0 * t.drag_handle_vertical_padding));
    CHECK(s.drag_handle()->accessible().description == widgets::english_us_patterns().expand_bottom_sheet);
    s.handle(clock.at(PointerAction::down, h.x + 16.0, h.y + 24.0));
    s.handle(clock.at(PointerAction::up, h.x + 16.0, h.y + 24.0));
    CHECK(s.target_value() == SheetValue::expanded);
    run(s, clock);
    const render::Rect top = s.drag_handle()->bounds();
    s.handle(clock.at(PointerAction::down, top.x + 16.0, top.y + 24.0));
    const auto r = s.handle(clock.at(PointerAction::up, top.x + 16.0, top.y + 24.0));
    CHECK(r.dismissed);
}

TEST_CASE("a sheet opened from the keyboard focuses its drag handle and keeps Tab inside") {
    Content c(2, 8.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show(input::InputSource::keyboard);
    run(s, clock);
    CHECK(s.drag_handle()->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(c.buttons[0]->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(c.buttons[1]->focused());
    s.handle(input::KeyEvent{Key::tab, false});
    CHECK(s.drag_handle()->focused());
    s.handle(input::KeyEvent{Key::tab, true});
    CHECK(c.buttons[1]->focused());
    s.handle(input::KeyEvent{Key::space, false});
    CHECK(c.clicks[1] == 1);
    s.handle(input::KeyEvent{Key::tab, false});
    s.handle(input::KeyEvent{Key::enter, false});
    CHECK(s.target_value() == SheetValue::expanded);
}

TEST_CASE("skipping the partially expanded state opens a sheet fully") {
    Content c(3, 8.0);
    widgets::BottomSheet s({.content = c.slot(), .skip_partially_expanded = true});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    CHECK(s.current_value() == SheetValue::expanded);
    CHECK(s.offset() == doctest::Approx(area.height - s.sheet_height()));
}

TEST_CASE("a standard sheet peeks, expands from its handle and never hides by default") {
    Content c(10, 16.0);
    widgets::BottomSheet s({.kind = widgets::BottomSheetKind::standard, .content = c.slot(), .initial = SheetValue::partially_expanded});
    s.set_area(area);
    CHECK(s.visible());
    CHECK(s.offset() == doctest::Approx(area.height - t.peek_height));
    CHECK(s.body_area().height == doctest::Approx(area.height - t.peek_height));
    CHECK_FALSE(s.has_state(SheetValue::hidden));
    CHECK(s.scrim_alpha() == doctest::Approx(0.0));
    Clock clock;
    const render::Rect h = s.drag_handle()->bounds();
    s.handle(clock.at(PointerAction::down, h.x + 16.0, h.y + 24.0));
    s.handle(clock.at(PointerAction::up, h.x + 16.0, h.y + 24.0));
    run(s, clock);
    CHECK(s.current_value() == SheetValue::expanded);
    const render::Rect top = s.drag_handle()->bounds();
    s.handle(clock.at(PointerAction::down, top.x + 16.0, top.y + 24.0));
    s.handle(clock.at(PointerAction::up, top.x + 16.0, top.y + 24.0));
    run(s, clock);
    CHECK(s.current_value() == SheetValue::partially_expanded);
    drag(s, clock, 200.0, area.height - 30.0, area.height - 2.0, 0.1);
    run(s, clock);
    CHECK(s.current_value() == SheetValue::partially_expanded);
}

TEST_CASE("a standard sheet lets events beside it through and a modal one does not") {
    Content c(3, 8.0);
    widgets::BottomSheet standard({.kind = widgets::BottomSheetKind::standard, .content = c.slot(), .initial = SheetValue::partially_expanded});
    standard.set_area({0.0, 0.0, 1000.0, 800.0});
    Clock clock;
    CHECK_FALSE(standard.handle(clock.at(PointerAction::down, 10.0, 100.0)).consumed);
    standard.handle(clock.at(PointerAction::up, 10.0, 100.0));
    Content d(3, 8.0);
    widgets::BottomSheet m({.content = d.slot()});
    m.set_area({0.0, 0.0, 1000.0, 800.0});
    m.show();
    run(m, clock);
    CHECK(m.handle(clock.at(PointerAction::down, 10.0, 100.0)).consumed);
}

TEST_CASE("the sheet reports a dialog named after the Compose pane title") {
    Content c(2, 8.0);
    widgets::BottomSheet s({.content = c.slot(), .strings = &widgets::japanese_patterns()});
    s.set_area(area);
    CHECK_FALSE(s.accessible());
    s.show();
    Clock clock;
    run(s, clock);
    const auto node = s.accessible();
    REQUIRE(node);
    CHECK(node->role == widgets::AccessibleRole::dialog);
    CHECK(node->name == "ボトムシート");
    CHECK(node->state.modal);
    CHECK(widgets::find_accessible(node->children, "ドラッグ ハンドル"));
    CHECK(widgets::find_accessible(node->children, "シートを閉じる"));
    CHECK(widgets::find_accessible(node->children, "Item 1"));
}

namespace {

void touch_drag(widgets::BottomSheet& s, Clock& clock, double x, double from, double to, double seconds) {
    auto at = [&](PointerAction a, double y) { return input::PointerEvent{a, x, y, 0, input::PointerKind::touch, clock.now}; };
    s.handle(at(PointerAction::down, from));
    const int n = std::max(2, static_cast<int>(seconds / frame_seconds));
    for (int i = 1; i <= n; ++i) {
        clock.now += frame_seconds;
        s.handle(at(PointerAction::move, from + (to - from) * i / n));
        s.step(frame_seconds);
    }
    s.handle(at(PointerAction::up, to));
}

}

TEST_CASE("a touch drag scrolls an expanded sheet's content where it can, and the handle still moves the sheet") {
    Content c(30, 16.0);
    widgets::BottomSheet s({.content = c.slot()});
    s.set_area(area);
    Clock clock;
    s.show();
    run(s, clock);
    touch_drag(s, clock, 30.0, 500.0, 300.0, 0.3);
    run(s, clock);
    REQUIRE(s.current_value() == SheetValue::expanded);
    CHECK(s.content().scroll().offset() == 0.0);
    touch_drag(s, clock, 30.0, 600.0, 300.0, 1.0);
    run(s, clock);
    CHECK(s.current_value() == SheetValue::expanded);
    const double scrolled = s.content().scroll().offset();
    CHECK(scrolled > 200.0);
    const render::Rect handle = s.bounds();
    touch_drag(s, clock, handle.x + handle.width / 2.0, handle.y + 20.0, handle.y + 500.0, 1.0);
    run(s, clock);
    CHECK(s.current_value() != SheetValue::expanded);
    CHECK(s.content().scroll().offset() == doctest::Approx(scrolled));
}
