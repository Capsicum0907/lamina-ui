#include "lamina/widgets/calendar_grid.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::DatePickerTokens& t = tokens::date_picker_tokens;
constexpr const tokens::DatePickerColors& k = tokens::date_picker_colors;

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

color::Argb dim(color::Argb c, double enabled) {
    return color::mix(color::scale_alpha(c, k.disabled_opacity), c, enabled);
}

}

double month_grid_width() {
    return calendar::days_per_week * t.cell_size;
}

double month_grid_height() {
    return t.rows * t.cell_size;
}

render::Rect day_rect(render::Point o, int cell) {
    const int col = cell % calendar::days_per_week;
    const int row = cell / calendar::days_per_week;
    return {o.x + col * t.cell_size, o.y + row * t.cell_size, t.cell_size, t.cell_size};
}

std::optional<calendar::Date> day_at(render::Point o, const calendar::YearMonth& month, calendar::Weekday first_day, double x, double y) {
    const double lx = x - o.x;
    const double ly = y - o.y;
    if (lx < 0.0 || ly < 0.0 || lx >= month_grid_width() || ly >= month_grid_height()) {
        return std::nullopt;
    }
    const int cell = static_cast<int>(ly / t.cell_size) * calendar::days_per_week + static_cast<int>(lx / t.cell_size);
    return calendar::date_in_cell(month, first_day, cell);
}

bool day_enabled(const MonthGridState& s, const calendar::Date& d) {
    return s.years.contains(d.year) && (!s.selectable || s.selectable->allows(d));
}

std::string day_description(const calendar::CalendarLocale& locale, const calendar::Date& d, const calendar::Date& today,
                            const DaySelection& selection) {
    std::string prefix;
    if (selection.range) {
        if (selection.start == d) {
            prefix = locale.strings.range_start_headline;
        } else if (selection.end == d) {
            prefix = locale.strings.range_end_headline;
        } else if (selection.in_range(d)) {
            prefix = locale.strings.day_in_range;
        }
    }
    if (d == today) {
        prefix = prefix.empty() ? locale.strings.today_description : prefix + locale.strings.list_separator + locale.strings.today_description;
    }
    const std::string date = calendar::format_date(locale.long_date_pattern, d, locale.names);
    return prefix.empty() ? date : prefix + locale.strings.list_separator + date;
}

void paint_weekdays(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts,
                    const calendar::CalendarLocale& locale, render::Point o) {
    const text::TextStyle style = style_of(t.weekday_type);
    for (int i = 0; i < calendar::days_per_week; ++i) {
        const calendar::Weekday w = calendar::weekday_from_index(calendar::weekday_index(locale.first_day_of_week) + i);
        const text::TextLine line =
            text::layout_line(fonts, locale.names.weekdays_narrow[static_cast<std::size_t>(calendar::weekday_index(w))], style);
        const double x = o.x + i * t.cell_size + (t.cell_size - line.width) / 2.0;
        text::draw_line(canvas, fonts, line, {x, o.y + (t.cell_size - line.box_height()) / 2.0}, scheme[k.weekday]);
    }
}

std::vector<AccessibleNode> weekday_accessible(const calendar::CalendarLocale& locale, render::Point o) {
    std::vector<AccessibleNode> out;
    for (int i = 0; i < calendar::days_per_week; ++i) {
        const calendar::Weekday w = calendar::weekday_from_index(calendar::weekday_index(locale.first_day_of_week) + i);
        out.push_back({AccessibleRole::label, locale.names.weekdays_long[static_cast<std::size_t>(calendar::weekday_index(w))],
                       {o.x + i * t.cell_size, o.y, t.cell_size, t.cell_size}});
    }
    return out;
}

void paint_month(render::Canvas& canvas, const color::ColorScheme& scheme, const text::FontStack& fonts, render::Point o,
                 const calendar::YearMonth& month, const MonthGridState& s) {
    const calendar::Weekday first = s.locale->first_day_of_week;
    const DaySelection& sel = s.selection;
    if (sel.range && sel.start && sel.end) {
        const calendar::Date from = std::max(*sel.start, calendar::first_day(month));
        const calendar::Date to = std::min(*sel.end, calendar::last_day(month));
        if (from <= to) {
            const int a = calendar::cell_of(from, first);
            const int b = calendar::cell_of(to, first);
            const int row_a = a / calendar::days_per_week;
            const int row_b = b / calendar::days_per_week;
            const double half = t.cell_size / 2.0;
            const double start_x = (a % calendar::days_per_week) * t.cell_size + (from == *sel.start ? half : 0.0);
            const double end_x = (b % calendar::days_per_week) * t.cell_size + (to == *sel.end ? half : t.cell_size);
            const double inset = (t.cell_size - t.range_band_height) / 2.0;
            const color::Argb band = dim(scheme[k.in_range_container], s.enabled);
            for (int row = row_a; row <= row_b; ++row) {
                const double x0 = row == row_a ? start_x : 0.0;
                const double x1 = row == row_b ? end_x : month_grid_width();
                if (x1 > x0) {
                    canvas.fill_rect({o.x + x0, o.y + row * t.cell_size + inset, x1 - x0, t.range_band_height}, band);
                }
            }
        }
    }
    const text::TextStyle style = style_of(t.day_type);
    const int lead = calendar::leading_days(month, first);
    const int days = calendar::days_in_month(month.year, month.month);
    for (int day = 1; day <= days; ++day) {
        const calendar::Date d{month.year, month.month, day};
        const render::Rect cell = day_rect(o, lead + day - 1);
        const render::Point c{cell.x + cell.width / 2.0, cell.y + cell.height / 2.0};
        const double r = t.day_size / 2.0;
        const bool enabled = day_enabled(s, d);
        const bool selected = sel.selected(d);
        const bool in_range = !selected && sel.in_range(d);
        const bool today = d == s.today;
        const double e = enabled ? s.enabled : 0.0;
        color::Argb content = scheme[k.day];
        if (selected) {
            content = scheme[k.selected_day];
        } else if (in_range) {
            content = enabled ? scheme[k.in_range] : scheme[k.day];
        } else if (today && enabled) {
            content = scheme[k.today];
        }
        content = dim(content, e);
        if (selected) {
            const double alpha = s.fading == d ? std::clamp(s.fade, 0.0, 1.0) : 1.0;
            canvas.fill_path(render::circle_path(c, r), color::scale_alpha(dim(scheme[k.selected_day_container], e), alpha));
            if (alpha < 1.0) {
                content = color::mix(dim(today && enabled ? scheme[k.today] : scheme[k.day], e), content, alpha);
            }
        } else if (today) {
            canvas.fill_path(render::ring_path(render::circle_rect(c, r), r, t.today_outline), dim(scheme[k.today_outline], e),
                             render::FillRule::even_odd);
        }
        if (enabled && s.hovered == d && s.hover_opacity > 0.0) {
            canvas.fill_path(render::circle_path(c, r), color::with_alpha(content, s.hover_opacity));
        }
        const text::TextLine line = text::layout_line(fonts, calendar::format_number(day), style);
        text::draw_line(canvas, fonts, line, {c.x - line.width / 2.0, c.y - line.box_height() / 2.0}, content);
        if (s.focus && s.focused == d) {
            s.focus->paint_circle(canvas, scheme, c, r);
        }
    }
}

void month_accessible(std::vector<AccessibleNode>& out, render::Point o, const calendar::YearMonth& month, const MonthGridState& s) {
    const calendar::Weekday first = s.locale->first_day_of_week;
    const int lead = calendar::leading_days(month, first);
    const int days = calendar::days_in_month(month.year, month.month);
    for (int day = 1; day <= days; ++day) {
        const calendar::Date d{month.year, month.month, day};
        const render::Rect cell = day_rect(o, lead + day - 1);
        const double inset = (t.cell_size - t.day_size) / 2.0;
        const bool enabled = day_enabled(s, d) && s.enabled > 0.5;
        out.push_back({AccessibleRole::button, day_description(*s.locale, d, s.today, s.selection),
                       {cell.x + inset, cell.y + inset, t.day_size, t.day_size},
                       {.enabled = enabled, .focused = s.focus && s.focus->focused() && s.focused == d, .selected = s.selection.selected(d)}});
    }
}

std::optional<calendar::Date> grid_key_target(const input::KeyEvent& e, const calendar::Date& f, calendar::Weekday first_day) {
    if (e.ctrl) {
        return std::nullopt;
    }
    switch (e.key) {
    case input::Key::left: return calendar::add_days(f, -1);
    case input::Key::right: return calendar::add_days(f, 1);
    case input::Key::up: return calendar::add_days(f, -calendar::days_per_week);
    case input::Key::down: return calendar::add_days(f, calendar::days_per_week);
    case input::Key::page_up: return calendar::add_months(f, e.shift ? -calendar::months_per_year : -1);
    case input::Key::page_down: return calendar::add_months(f, e.shift ? calendar::months_per_year : 1);
    case input::Key::home: return calendar::add_days(f, -calendar::column_of(calendar::weekday(f), first_day));
    case input::Key::end: return calendar::add_days(f, calendar::days_per_week - 1 - calendar::column_of(calendar::weekday(f), first_day));
    default: return std::nullopt;
    }
}

calendar::Date clamp_to_years(const calendar::Date& d, const calendar::YearRange& years) {
    if (d.year < years.first) {
        return {years.first, 1, 1};
    }
    if (d.year > years.last) {
        return {years.last, 12, 31};
    }
    return d;
}

}
