#include <doctest/doctest.h>

#include "lamina/calendar/locale.hpp"
#include "lamina/widgets/date_picker.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using calendar::Date;
using input::Key;
using input::PointerAction;
using widgets::DatePicker;
using widgets::DatePickerMode;
using widgets::DatePickerOptions;

namespace {

constexpr Date today{2026, 11, 21};

DatePickerOptions options(std::optional<Date> selected = std::nullopt) {
    DatePickerOptions o{.today = today, .selected = selected};
    return o;
}

void tap(DatePicker& p, render::Rect r) {
    const double x = r.x + r.width / 2.0;
    const double y = r.y + r.height / 2.0;
    p.handle({PointerAction::down, x, y});
    p.handle({PointerAction::up, x, y});
    p.handle({PointerAction::leave, 0.0, 0.0});
}

int distance(color::Argb a, color::Argb b) {
    int d = 0;
    for (int shift : {0, 8, 16}) {
        d += std::abs(static_cast<int>((a >> shift) & 0xff) - static_cast<int>((b >> shift) & 0xff));
    }
    return d;
}

void type(DatePicker& p, const char* text) {
    p.handle(input::TextEvent{input::TextEventKind::commit, text, 0});
}

const widgets::AccessibleNode* find(const widgets::AccessibleNode& n, const std::string& name) {
    if (n.name == name) {
        return &n;
    }
    for (const auto& c : n.children) {
        if (const auto* f = find(c, name)) {
            return f;
        }
    }
    return nullptr;
}

}

static_assert(widgets::Measurable<DatePicker>);
static_assert(widgets::Disableable<DatePicker>);

TEST_CASE("date picker tokens follow DatePickerModalTokens.kt and DatePicker.kt") {
    const auto& t = tokens::date_picker_tokens;
    CHECK(t.container_width == 360.0);
    CHECK(t.container_height == 568.0);
    CHECK(t.header_height == 120.0);
    CHECK(t.cell_size == 48.0);
    CHECK(t.day_size == 40.0);
    CHECK(t.year_width == 72.0);
    CHECK(t.year_height == 36.0);
    CHECK(t.year_columns == 3);
    CHECK(t.navigation_height == 56.0);
    CHECK(t.headline_type == tokens::TypeRole::headline_large);
    const auto& c = tokens::date_picker_colors;
    CHECK(c.container == color::Role::surface_container_high);
    CHECK(c.selected_day_container == color::Role::primary);
    CHECK(c.today_outline == color::Role::primary);
    CHECK(c.in_range_container == color::Role::secondary_container);
}

TEST_CASE("the picker shows the selected date's month and its headline") {
    DatePicker p(options(Date{2026, 3, 27}), fonts(), icon_font());
    CHECK(p.displayed_month() == calendar::YearMonth{2026, 3});
    CHECK(p.headline() == "Mar 27, 2026");
    CHECK(p.title() == "Select date");
    CHECK(p.height() == doctest::Approx(121.0 + 56.0 + 48.0 + 288.0));
    DatePicker empty(options(), fonts(), icon_font());
    CHECK(empty.displayed_month() == calendar::YearMonth{2026, 11});
    CHECK(empty.headline() == "Selected date");
}

TEST_CASE("a click on a day selects it, and days the predicate refuses stay unselected") {
    DatePickerOptions o = options();
    o.selectable.date = [](const Date& d) { return calendar::weekday(d) != calendar::Weekday::sunday; };
    DatePicker p(std::move(o), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    tap(p, p.day_bounds({2026, 11, 22}));
    CHECK_FALSE(p.selected());
    tap(p, p.day_bounds({2026, 11, 24}));
    CHECK(p.selected() == Date{2026, 11, 24});
    settle(p);
    thorvg::Canvas canvas(360, 520);
    canvas.clear(0xff000000);
    p.paint(canvas, scheme());
    canvas.finish();
    const render::Rect b = p.day_bounds({2026, 11, 24});
    CHECK(canvas.pixel(static_cast<int>(b.x + 4.0), static_cast<int>(b.y + b.height / 2.0)) == scheme()[color::Role::primary]);
    const render::Rect today_box = p.day_bounds(today);
    const color::Argb ring = canvas.pixel(static_cast<int>(today_box.x + today_box.width / 2.0), static_cast<int>(today_box.y));
    CHECK(distance(ring, scheme()[color::Role::primary]) < distance(ring, scheme()[color::Role::surface_container_high]));
}

TEST_CASE("the arrows page through months with a slide, and the year picker keeps the month") {
    DatePicker p(options(), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    p.step(frame_seconds);
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    const auto nodes = p.accessible();
    const auto* next = find(nodes, "Change to next month");
    REQUIRE(next);
    tap(p, next->bounds);
    CHECK(p.displayed_month() == calendar::YearMonth{2026, 12});
    p.step(frame_seconds);
    CHECK(p.month_slide() > 0.0);
    settle(p);
    CHECK(p.month_slide() == doctest::Approx(0.0));
    const auto* menu = find(p.accessible(), "December 2026");
    REQUIRE(menu);
    tap(p, menu->bounds);
    CHECK(p.year_picker_open());
    settle(p);
    CHECK(p.year_presence() == doctest::Approx(1.0));
    p.step(frame_seconds);
    tap(p, p.year_bounds(2024));
    CHECK_FALSE(p.year_picker_open());
    CHECK(p.displayed_month() == calendar::YearMonth{2024, 12});
}

TEST_CASE("the day grid is one Tab stop with arrows, Page Up and Down, Home and End and Enter") {
    DatePickerOptions o = options(Date{2026, 11, 5});
    o.locale = &calendar::english_gb();
    DatePicker p(std::move(o), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    for (int i = 0; i < 5 && !p.focused_day(); ++i) {
        focus.handle({Key::tab});
    }
    REQUIRE(p.focused_day());
    CHECK(*p.focused_day() == Date{2026, 11, 5});
    p.handle(input::KeyEvent{Key::down});
    CHECK(*p.focused_day() == Date{2026, 11, 12});
    p.handle(input::KeyEvent{Key::home});
    CHECK(*p.focused_day() == Date{2026, 11, 9});
    p.handle(input::KeyEvent{Key::end});
    CHECK(*p.focused_day() == Date{2026, 11, 15});
    p.handle(input::KeyEvent{Key::page_down});
    CHECK(*p.focused_day() == Date{2026, 12, 15});
    CHECK(p.displayed_month() == calendar::YearMonth{2026, 12});
    p.handle(input::KeyEvent{Key::page_up, true});
    CHECK(*p.focused_day() == Date{2025, 12, 15});
    p.handle(input::KeyEvent{Key::right});
    CHECK(p.handle(input::KeyEvent{Key::enter}).changed);
    CHECK(p.selected() == Date{2025, 12, 16});
    CHECK(p.headline() == "16 Dec 2025");
}

TEST_CASE("the text input mode parses the locale's pattern and reports Compose's errors") {
    DatePickerOptions o = options();
    o.mode = DatePickerMode::input;
    o.year_range = {2000, 2030};
    o.selectable.date = [](const Date& d) { return d.day != 13; };
    DatePicker p(std::move(o), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    CHECK(p.title() == "Select date");
    CHECK(p.headline() == "Entered date");
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    for (int i = 0; i < 4 && !p.input_field().focused(); ++i) {
        focus.handle({Key::tab});
    }
    REQUIRE(p.input_field().focused());
    type(p, "11212026");
    CHECK(p.input_text() == "11/21/2026");
    CHECK(p.selected() == Date{2026, 11, 21});
    CHECK(p.input_error().empty());
    p.handle(input::KeyEvent{Key::backspace});
    CHECK_FALSE(p.selected());
    type(p, "7");
    CHECK(p.input_error().empty());
    p.handle(input::KeyEvent{Key::a, false, true});
    type(p, "13012026");
    CHECK(p.input_error() == "Date does not match expected pattern: MM/DD/YYYY");
    p.handle(input::KeyEvent{Key::a, false, true});
    type(p, "01011999");
    CHECK(p.input_error() == "Date out of expected year range 2000 - 2030");
    CHECK(p.input_field().error());
    p.handle(input::KeyEvent{Key::a, false, true});
    type(p, "03132026");
    CHECK(p.input_error() == "Date not allowed: Mar 13, 2026");
    CHECK_FALSE(p.selected());
}

TEST_CASE("Japanese uses its own pattern, names and messages") {
    DatePickerOptions o = options(Date{2026, 11, 21});
    o.locale = &calendar::japanese();
    DatePicker p(std::move(o), fonts(), icon_font());
    CHECK(p.headline() == "2026年11月21日");
    CHECK(p.title() == "日付の選択");
    p.set_mode(DatePickerMode::input);
    CHECK(p.input_text() == "2026/11/21");
    CHECK(p.input_field().accessible().name == "日付、YYYY/MM/DD");
}

TEST_CASE("switching modes slides and fades, and the height follows") {
    DatePicker p(options(), fonts(), icon_font());
    const double picker = p.height();
    p.set_mode(DatePickerMode::input);
    p.step(frame_seconds);
    CHECK(p.mode_progress() > 0.0);
    CHECK(p.mode_progress() < 1.0);
    settle(p);
    CHECK(p.height() == doctest::Approx(121.0 + 10.0 + 56.0 + 16.0));
    CHECK(p.height() < picker);
    CHECK(p.input_field().focused());
}

TEST_CASE("days are buttons named by their full date, with today and the selection") {
    DatePicker p(options(Date{2026, 11, 20}), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    const auto n = p.accessible();
    const auto* t = find(n, "Today, Saturday, November 21, 2026");
    REQUIRE(t);
    CHECK(t->role == widgets::AccessibleRole::button);
    const auto* s = find(n, "Friday, November 20, 2026");
    REQUIRE(s);
    CHECK(s->state.selected == true);
    CHECK(find(n, "Sunday"));
}

TEST_CASE("a disabled picker takes no clicks or focus") {
    DatePicker p(options(), fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    p.set_enabled(false);
    tap(p, p.day_bounds({2026, 11, 3}));
    CHECK_FALSE(p.selected());
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    CHECK_FALSE(focus.any_can_focus());
}
