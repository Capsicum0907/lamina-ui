#include <doctest/doctest.h>

#include <cmath>
#include <numbers>

#include "lamina/calendar/locale.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/picker_dialog.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/time_picker.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::TimePicker;
using widgets::TimePickerMode;
using widgets::TimeSelection;

namespace {

render::Point at(const TimePicker& p, double degrees, double radius) {
    const render::Point c = p.dial_center();
    const double r = degrees * std::numbers::pi / 180.0;
    return {c.x + radius * std::sin(r), c.y - radius * std::cos(r)};
}

void tap(TimePicker& p, render::Point pt) {
    p.handle({PointerAction::down, pt.x, pt.y});
    p.handle({PointerAction::up, pt.x, pt.y});
    p.handle({PointerAction::leave, 0.0, 0.0});
}

void tap(TimePicker& p, render::Rect r) {
    tap(p, render::Point{r.x + r.width / 2.0, r.y + r.height / 2.0});
}

void type(TimePicker& p, const char* text) {
    p.handle(input::TextEvent{input::TextEventKind::commit, text, 0});
}

TimePicker make(widgets::TimePickerOptions o) {
    return TimePicker(o, fonts());
}

}

static_assert(widgets::Measurable<TimePicker>);
static_assert(widgets::Disableable<TimePicker>);

TEST_CASE("time picker tokens follow TimePickerTokens.kt, TimeInputTokens.kt and TimePicker.kt") {
    const auto& t = tokens::time_picker_tokens;
    CHECK(t.dial_size == 256.0);
    CHECK(t.outer_ratio * 256.0 == doctest::Approx(101.0));
    CHECK(t.inner_ratio * 256.0 == doctest::Approx(69.0));
    CHECK(t.inner_ring_distance == 74.0);
    CHECK(t.handle_size == 48.0);
    CHECK(t.track_width == 2.0);
    CHECK(t.center_size == 8.0);
    CHECK(t.selector_width == 96.0);
    CHECK(t.selector_height == 80.0);
    CHECK(t.period_vertical_width == 52.0);
    CHECK(t.period_horizontal_width == 216.0);
    CHECK(t.input_field_height == 72.0);
    const auto& c = tokens::time_picker_colors;
    CHECK(c.dial == color::Role::surface_container_highest);
    CHECK(c.selector == color::Role::primary);
    CHECK(c.period_selected_container == color::Role::primary_container);
    CHECK(c.time_selected_container == color::Role::primary_container);
}

TEST_CASE("the vertical and horizontal layouts place the display and the dial as Compose does") {
    TimePicker v = make({.hour = 9, .minute = 41, .is_24_hour = false});
    v.set_origin({0.0, 0.0});
    CHECK(v.width() == 272.0);
    CHECK(v.height() == doctest::Approx(80.0 + 36.0 + 256.0 + 24.0));
    CHECK(v.period_bounds(false).height == doctest::Approx(38.0));
    CHECK(v.dial_center().y == doctest::Approx(80.0 + 36.0 + 128.0));
    TimePicker h = make({.hour = 9, .minute = 41, .is_24_hour = false, .layout = widgets::TimePickerLayout::horizontal});
    h.set_origin({0.0, 0.0});
    CHECK(h.width() == doctest::Approx(216.0 + 36.0 + 256.0));
    CHECK(h.height() == 256.0);
    CHECK(h.period_bounds(true).width == doctest::Approx(106.0));
    TimePicker day = make({.hour = 9, .minute = 41, .is_24_hour = true});
    CHECK(day.width() == 256.0);
}

TEST_CASE("a tap on the dial picks the hour, then switches to minutes after the selector settles") {
    TimePicker p = make({.hour = 9, .minute = 41, .is_24_hour = false});
    p.set_origin({0.0, 0.0});
    CHECK(p.display_hour() == 9);
    tap(p, at(p, 90.0, 101.0));
    CHECK(p.hour() == 3);
    CHECK(p.selection() == TimeSelection::hour);
    p.step(frame_seconds);
    CHECK(p.selector_angle() > 90.0);
    settle(p);
    CHECK(p.selection() == TimeSelection::minute);
    CHECK(std::fmod(p.selector_angle() + 3600.0, 360.0) == doctest::Approx(41.0 * 6.0));
    tap(p, at(p, 100.0, 101.0));
    CHECK(p.minute() == 15);
    settle(p);
    CHECK(p.selection() == TimeSelection::minute);
}

TEST_CASE("dragging follows the pointer to the minute and AM and PM keep the hour") {
    TimePicker p = make({.hour = 14, .minute = 0, .is_24_hour = false});
    p.set_origin({0.0, 0.0});
    p.set_selection(TimeSelection::minute);
    settle(p);
    const render::Point a = at(p, 10.0, 100.0);
    p.handle({PointerAction::down, a.x, a.y});
    const render::Point b = at(p, 44.0, 100.0);
    p.handle({PointerAction::move, b.x, b.y});
    CHECK(p.minute() == 7);
    CHECK(p.selector_angle() == doctest::Approx(44.0));
    p.handle({PointerAction::up, b.x, b.y});
    settle(p);
    CHECK(p.selector_angle() == doctest::Approx(42.0));
    CHECK(p.is_pm());
    tap(p, p.period_bounds(false));
    CHECK(p.hour() == 2);
    CHECK(p.formatted() == "2:07 AM");
}

TEST_CASE("in 24-hour mode the inner ring holds 12 to 23") {
    TimePicker p = make({.hour = 8, .minute = 0, .is_24_hour = true});
    p.set_origin({0.0, 0.0});
    CHECK(p.dial_labels().size() == 24);
    tap(p, at(p, 60.0, 69.0));
    CHECK(p.hour() == 14);
    TimePicker q = make({.hour = 8, .minute = 0, .is_24_hour = true});
    q.set_origin({0.0, 0.0});
    tap(q, at(q, 0.0, 101.0));
    CHECK(q.hour() == 0);
    CHECK(q.formatted() == "00:00");
}

TEST_CASE("the selectors, the period and the dial are Tab stops with their own keys") {
    TimePicker p = make({.hour = 9, .minute = 41, .is_24_hour = false});
    p.set_origin({0.0, 0.0});
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    focus.handle({Key::tab});
    focus.handle({Key::tab});
    CHECK(p.handle(input::KeyEvent{Key::space}).consumed);
    CHECK(p.selection() == TimeSelection::minute);
    focus.handle({Key::tab});
    p.handle(input::KeyEvent{Key::down});
    CHECK(p.is_pm());
    focus.handle({Key::tab});
    p.handle(input::KeyEvent{Key::right});
    p.handle(input::KeyEvent{Key::enter});
    CHECK(p.minute() == 45);
}

TEST_CASE("the input mode takes two digits per field, advances to minutes and reports Compose's errors") {
    TimePicker p = make({.hour = 9, .minute = 41, .is_24_hour = false, .mode = TimePickerMode::input});
    p.set_origin({0.0, 0.0});
    CHECK(p.height() == doctest::Approx(72.0 + 7.0 + 32.0));
    CHECK(p.hour_text() == "09");
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    focus.handle({Key::tab});
    type(p, "1");
    CHECK(p.hour_text() == "1");
    CHECK(p.selection() == TimeSelection::hour);
    type(p, "3");
    CHECK(p.hour_error() == "Hour must be 1–12");
    p.handle(input::KeyEvent{Key::backspace});
    type(p, "1");
    CHECK(p.hour() == 11);
    CHECK(p.selection() == TimeSelection::minute);
    type(p, "75");
    CHECK(p.minute_error() == "Minute must be 0–59");
    CHECK(p.minute() == 41);
    type(p, "5");
    CHECK(p.minute_text() == "5");
    CHECK(p.minute() == 5);
    CHECK(p.formatted() == "11:05 AM");
}

TEST_CASE("Japanese uses the 24-hour clock and its own names") {
    widgets::TimePickerOptions o{.hour = 19, .minute = 5};
    o.locale = &calendar::japanese();
    TimePicker p(o, fonts());
    p.set_origin({0.0, 0.0});
    CHECK(p.is_24_hour());
    CHECK(p.formatted() == "19:05");
    const widgets::AccessibleNode n = p.accessible();
    CHECK(n.children[0].name == "時刻を選択");
    bool found = false;
    widgets::visit_accessible({n}, [&](const widgets::AccessibleNode& c) { found = found || c.name == "19 時間"; });
    CHECK(found);
}

TEST_CASE("the time picker dialog puts the title above and the mode toggle beside the actions") {
    TimePicker p = make({.hour = 9, .minute = 41, .is_24_hour = false});
    bool toggled = false;
    widgets::PickerDialog d({"OK", "Cancel", widgets::PickerDialogLayout::time, tokens::CornerSize::extra_large, "Select Time",
                             widgets::PickerDialogLeading{{icon_font(), res::icons::edit}, "Switch to text input mode",
                                                          [&](widgets::IconButton&) { toggled = true; }}},
                            std::make_unique<widgets::PaneOf<TimePicker>>(p), fonts());
    d.open_in({0.0, 0.0, 900.0, 800.0});
    settle(d);
    CHECK(d.bounds().width == doctest::Approx(272.0 + 48.0));
    REQUIRE(d.leading());
    CHECK(d.leading()->bounds().x == doctest::Approx(d.bounds().x + 24.0));
    const render::Rect lead = d.leading()->bounds();
    d.handle(input::PointerEvent{PointerAction::down, lead.x + 10.0, lead.y + 10.0});
    d.handle(input::PointerEvent{PointerAction::up, lead.x + 10.0, lead.y + 10.0});
    CHECK(toggled);
}

TEST_CASE("a disabled time picker takes no input") {
    TimePicker p = make({.hour = 9, .minute = 41, .is_24_hour = false, .enabled = false});
    p.set_origin({0.0, 0.0});
    tap(p, at(p, 90.0, 101.0));
    CHECK(p.hour() == 9);
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    CHECK_FALSE(focus.any_can_focus());
}
