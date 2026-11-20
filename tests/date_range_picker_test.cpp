#include <doctest/doctest.h>

#include "lamina/calendar/locale.hpp"
#include "lamina/widgets/date_picker.hpp"
#include "lamina/widgets/date_range_picker.hpp"
#include "lamina/widgets/docked_date_picker.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/picker_dialog.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using calendar::Date;
using input::Key;
using input::PointerAction;
using widgets::DatePickerMode;

namespace {

constexpr Date today{2026, 11, 21};

template <typename W>
void tap(W& w, render::Rect r) {
    const double x = r.x + r.width / 2.0;
    const double y = r.y + r.height / 2.0;
    w.handle(input::PointerEvent{PointerAction::down, x, y});
    w.handle(input::PointerEvent{PointerAction::up, x, y});
    w.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0});
}

}

static_assert(widgets::Measurable<widgets::DateRangePicker>);
static_assert(widgets::Measurable<widgets::DockedDatePicker>);

TEST_CASE("range clicks set the start, then the end, and start again before the start or after an end") {
    widgets::DateRangePicker p({.today = today}, fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    CHECK(p.headline() == "Start date - End date");
    CHECK(p.first_visible_month() == calendar::YearMonth{2026, 11});
    tap(p, p.day_bounds({2026, 11, 10}));
    CHECK(p.start() == Date{2026, 11, 10});
    CHECK_FALSE(p.end());
    tap(p, p.day_bounds({2026, 11, 7}));
    CHECK(p.start() == Date{2026, 11, 7});
    tap(p, p.day_bounds({2026, 11, 12}));
    CHECK(p.end() == Date{2026, 11, 12});
    CHECK(p.headline() == "Nov 7, 2026 - Nov 12, 2026");
    tap(p, p.day_bounds({2026, 11, 15}));
    CHECK(p.start() == Date{2026, 11, 15});
    CHECK_FALSE(p.end());
    tap(p, p.day_bounds({2026, 11, 15}));
    CHECK(p.end() == Date{2026, 11, 15});
}

TEST_CASE("the range band joins the start and end circles across rows") {
    widgets::DateRangePicker p({.today = today, .start = Date{2026, 11, 5}, .end = Date{2026, 11, 18}}, fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    settle(p);
    thorvg::Canvas canvas(360, 520);
    canvas.clear(0xff000000);
    p.paint(canvas, scheme());
    canvas.finish();
    const render::Rect mid = p.day_bounds({2026, 11, 10});
    CHECK(canvas.pixel(static_cast<int>(mid.x - 3.0), static_cast<int>(mid.y + 4.0)) == scheme()[color::Role::secondary_container]);
    const render::Rect start = p.day_bounds({2026, 11, 5});
    CHECK(canvas.pixel(static_cast<int>(start.x - 3.0), static_cast<int>(start.y + 20.0)) == scheme()[color::Role::surface_container_high]);
    CHECK(canvas.pixel(static_cast<int>(start.x + 44.0), static_cast<int>(start.y + 4.0)) == scheme()[color::Role::secondary_container]);
    const auto n = p.accessible();
    bool found = false;
    widgets::visit_accessible({n}, [&](const widgets::AccessibleNode& node) { found = found || node.name == "Start date, Thursday, November 5, 2026"; });
    CHECK(found);
}

TEST_CASE("the range input mode validates each field and the order of the dates") {
    widgets::DateRangePicker p({.today = today, .mode = DatePickerMode::input}, fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    CHECK(p.title() == "Enter dates");
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    focus.handle({Key::tab});
    focus.handle({Key::tab});
    REQUIRE(p.start_field().focused());
    p.handle(input::TextEvent{input::TextEventKind::commit, "11202026", 0});
    CHECK(p.start() == Date{2026, 11, 20});
    focus.handle({Key::tab});
    p.handle(input::TextEvent{input::TextEventKind::commit, "11102026", 0});
    CHECK(p.end_error() == "Invalid date range input");
    CHECK_FALSE(p.end());
    p.handle(input::KeyEvent{Key::a, false, true});
    p.handle(input::TextEvent{input::TextEventKind::commit, "11252026", 0});
    CHECK(p.end() == Date{2026, 11, 25});
    CHECK(p.end_error().empty());
}

TEST_CASE("the range grid takes the date grid keys and scrolls the focused month into view") {
    widgets::DateRangePicker p({.today = today, .start = Date{2026, 11, 3}}, fonts(), icon_font());
    p.set_origin({0.0, 0.0});
    widgets::FocusManager focus;
    p.add_focus_stops(focus);
    focus.handle({Key::tab});
    focus.handle({Key::tab});
    REQUIRE(p.focused_day());
    CHECK(*p.focused_day() == Date{2026, 11, 3});
    for (int i = 0; i < 3; ++i) {
        p.handle(input::KeyEvent{Key::page_down});
    }
    settle(p);
    CHECK(*p.focused_day() == Date{2027, 2, 3});
    CHECK(p.first_visible_month() >= calendar::YearMonth{2027, 1});
    CHECK(p.handle(input::KeyEvent{Key::enter}).changed);
    CHECK(p.end() == Date{2027, 2, 3});
}

TEST_CASE("the picker dialog holds a pane above dismiss and confirm and closes with a result") {
    widgets::DatePicker picker({.today = today}, fonts(), icon_font());
    widgets::PickerDialog dialog({"OK", "Cancel"}, std::make_unique<widgets::PaneOf<widgets::DatePicker>>(picker), fonts());
    dialog.open_in({0.0, 0.0, 800.0, 700.0}, input::InputSource::keyboard);
    settle(dialog);
    CHECK(dialog.bounds().width == 360.0);
    CHECK(dialog.bounds().height == doctest::Approx(picker.height() + 48.0));
    CHECK(dialog.confirm_bounds().x > dialog.dismiss_bounds().x);
    CHECK(picker.focused());
    tap(dialog, picker.day_bounds({2026, 11, 9}));
    CHECK(picker.selected() == Date{2026, 11, 9});
    const render::Rect ok = dialog.confirm_bounds();
    dialog.handle(input::PointerEvent{PointerAction::down, ok.x + 5.0, ok.y + 5.0});
    CHECK(dialog.handle(input::PointerEvent{PointerAction::up, ok.x + 5.0, ok.y + 5.0}) == widgets::DialogResult::confirmed);
    CHECK_FALSE(dialog.is_open());
    dialog.open_in({0.0, 0.0, 800.0, 700.0});
    CHECK(dialog.handle(input::KeyEvent{Key::escape}) == widgets::DialogResult::dismissed);
    CHECK_FALSE(dialog.accessible().has_value());
}

TEST_CASE("the docked picker opens below its field, fills the field from a click and closes") {
    widgets::DockedDatePicker d({.today = today}, fonts(), icon_font());
    d.set_origin({0.0, 0.0});
    tap(d, d.open_button().bounds());
    CHECK(d.is_open());
    settle(d);
    CHECK(d.popup_bounds().y > d.field().bounds().y + d.field().bounds().height);
    CHECK_FALSE(d.picker().focused());
    CHECK_FALSE(d.open_button().focused());
    {
        const render::Rect r = d.picker().day_bounds({2026, 11, 26});
        d.handle(input::PointerEvent{PointerAction::down, r.x + 20.0, r.y + 20.0});
        CHECK_FALSE(d.picker().focused());
        CHECK_FALSE(d.open_button().focused());
        d.handle(input::PointerEvent{PointerAction::up, r.x + 20.0, r.y + 20.0});
        CHECK_FALSE(d.open_button().focused());
        d.handle(input::PointerEvent{PointerAction::leave, 0.0, 0.0});
        CHECK_FALSE(d.open_button().focused());
    }
    CHECK(d.selected() == Date{2026, 11, 26});
    CHECK(d.field().display_text() == "11/26/2026");
    CHECK_FALSE(d.is_open());
    widgets::FocusManager focus;
    d.add_focus_stops(focus);
    CHECK_FALSE(d.field().focused());
    focus.handle({Key::tab});
    REQUIRE(d.field().focused());
    d.handle(input::KeyEvent{Key::a, false, true});
    d.handle(input::TextEvent{input::TextEventKind::commit, "12012026", 0});
    CHECK(d.selected() == Date{2026, 12, 1});
    d.handle(input::KeyEvent{Key::down, false, false, false, true});
    CHECK(d.is_open());
    CHECK(d.picker().focused_day() == Date{2026, 12, 1});
    d.handle(input::KeyEvent{Key::escape});
    CHECK_FALSE(d.is_open());
}
