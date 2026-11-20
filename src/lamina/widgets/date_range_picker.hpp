#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/calendar/date.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/date_picker.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/calendar_grid.hpp"
#include "lamina/widgets/date_picker.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

struct DateRangePickerOptions {
    calendar::Date today;
    std::optional<calendar::Date> start = std::nullopt;
    std::optional<calendar::Date> end = std::nullopt;
    std::optional<calendar::YearMonth> displayed_month = std::nullopt;
    calendar::YearRange year_range{1900, 2100};
    SelectableDates selectable = {};
    DatePickerMode mode = DatePickerMode::picker;
    bool show_mode_toggle = true;
    double height = tokens::date_picker_tokens.range_picker_height;
    const calendar::CalendarLocale* locale = nullptr;
    input::Clipboard* clipboard = nullptr;
    bool enabled = true;
};

class DateRangePicker {
public:
    DateRangePicker(DateRangePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);
    DateRangePicker(const DateRangePicker&) = delete;
    DateRangePicker& operator=(const DateRangePicker&) = delete;

    double width() const;
    double height() const { return options_.height; }
    void set_height(double height);
    void set_origin(render::Point origin);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;

    std::optional<calendar::Date> start() const { return start_; }
    std::optional<calendar::Date> end() const { return end_; }
    void set_selection(std::optional<calendar::Date> start, std::optional<calendar::Date> end);
    DatePickerMode mode() const { return mode_; }
    void set_mode(DatePickerMode mode);
    const calendar::CalendarLocale& locale() const { return *locale_; }
    void set_locale(const calendar::CalendarLocale& locale);
    calendar::YearMonth first_visible_month() const;
    void show_month(calendar::YearMonth month, bool animate);
    std::optional<calendar::Date> focused_day() const;
    std::string title() const;
    std::string headline() const;
    const std::string& start_error() const { return errors_[0]; }
    const std::string& end_error() const { return errors_[1]; }
    const TextField& start_field() const { return *fields_[0]; }
    const TextField& end_field() const { return *fields_[1]; }
    render::Rect day_bounds(const calendar::Date& date) const;
    double crossfade() const { return mode_alpha_.value(); }

    void add_focus_stops(FocusManager& focus);
    bool focused() const;
    void clear_focus();
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    DatePickerResponse handle(const input::PointerEvent& event);
    DatePickerResponse handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double header_height() const;
    double content_top() const;
    double item_height() const;
    render::Rect list_area() const;
    int month_index(calendar::YearMonth month) const;
    calendar::YearMonth month_at(int index) const;
    int month_count() const;
    render::Point grid_origin(int index) const;
    MonthGridState grid_state(bool with_pointer) const;
    void click(const calendar::Date& date, DatePickerResponse& response);
    void focus_day(const calendar::Date& date);
    void rebuild_inputs();
    void sync_inputs();
    void on_input_changed(std::size_t field, DatePickerResponse& response);
    void place_children();
    void paint_picker(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_inputs(render::Canvas& canvas, const color::ColorScheme& scheme) const;

    DateRangePickerOptions options_;
    text::FontStack fonts_;
    icons::IconFont icon_font_;
    const calendar::CalendarLocale* locale_;
    std::optional<calendar::Date> start_;
    std::optional<calendar::Date> end_;
    DatePickerMode mode_;
    render::Point origin_{0.0, 0.0};
    IconButton toggle_;
    std::unique_ptr<TextField> fields_[2];
    std::string errors_[2];
    FocusIndicator grid_focus_;
    calendar::Date focus_date_;
    std::optional<calendar::Date> hovered_;
    std::optional<calendar::Date> pressed_;
    motion::Animated state_layer_;
    ScrollArea list_;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    motion::Animated mode_alpha_;
    std::optional<calendar::Date> fading_;
    motion::Animated selection_fade_;
    Enablement enabled_;
};

}
