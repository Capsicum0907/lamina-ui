#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lamina/calendar/date.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/date_picker.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"

namespace lamina::widgets {

struct SelectableDates {
    std::function<bool(const calendar::Date&)> date = {};
    std::function<bool(int)> year = {};

    bool allows_year(int y) const { return !year || year(y); }
    bool allows(const calendar::Date& d) const { return allows_year(d.year) && (!date || date(d)); }
};

struct DaySelection {
    std::optional<calendar::Date> start;
    std::optional<calendar::Date> end;
    bool range = false;

    bool selected(const calendar::Date& d) const { return start == d || end == d; }
    bool in_range(const calendar::Date& d) const { return range && start && end && d >= *start && d <= *end; }
};

struct MonthGridState {
    const calendar::CalendarLocale* locale;
    calendar::Date today;
    DaySelection selection;
    const SelectableDates* selectable;
    calendar::YearRange years;
    std::optional<calendar::Date> hovered;
    double hover_opacity = 0.0;
    std::optional<calendar::Date> focused;
    const FocusIndicator* focus = nullptr;
    std::optional<calendar::Date> fading;
    double fade = 1.0;
    double enabled = 1.0;
};

double month_grid_width();
double month_grid_height();
render::Rect day_rect(render::Point grid_origin, int cell);
std::optional<calendar::Date> day_at(render::Point grid_origin, const calendar::YearMonth& month, calendar::Weekday first_day,
                                     double x, double y);
bool day_enabled(const MonthGridState& state, const calendar::Date& d);
std::string day_description(const calendar::CalendarLocale& locale, const calendar::Date& d, const calendar::Date& today,
                            const DaySelection& selection);

void paint_weekdays(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts,
                    const calendar::CalendarLocale& locale, render::Point origin, double enabled = 1.0);
void paint_month(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts, render::Point grid_origin,
                 const calendar::YearMonth& month, const MonthGridState& state);
void month_accessible(std::vector<AccessibleNode>& out, render::Point grid_origin, const calendar::YearMonth& month,
                      const MonthGridState& state);
std::vector<AccessibleNode> weekday_accessible(const calendar::CalendarLocale& locale, render::Point origin);

std::optional<calendar::Date> grid_key_target(const input::KeyEvent& e, const calendar::Date& focused, calendar::Weekday first_day);
calendar::Date clamp_to_years(const calendar::Date& d, const calendar::YearRange& years);

}
