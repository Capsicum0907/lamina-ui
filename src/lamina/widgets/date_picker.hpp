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
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

enum class DatePickerMode { picker, input };

struct DatePickerOptions {
    calendar::Date today;
    std::optional<calendar::Date> selected = std::nullopt;
    std::optional<calendar::YearMonth> displayed_month = std::nullopt;
    calendar::YearRange year_range{1900, 2100};
    SelectableDates selectable = {};
    DatePickerMode mode = DatePickerMode::picker;
    bool show_header = true;
    bool show_mode_toggle = true;
    const calendar::CalendarLocale* locale = nullptr;
    input::Clipboard* clipboard = nullptr;
    bool enabled = true;
};

struct DatePickerResponse {
    bool changed = false;
    bool consumed = false;
};

std::string validate_date_input(const calendar::CalendarLocale& locale, const std::string& digits, const calendar::YearRange& years,
                                const SelectableDates& selectable, std::optional<calendar::Date>* parsed);

class DatePicker {
public:
    DatePicker(DatePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);
    DatePicker(const DatePicker&) = delete;
    DatePicker& operator=(const DatePicker&) = delete;

    double width() const;
    double height() const;
    double target_height() const;
    void set_origin(render::Point origin);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;

    std::optional<calendar::Date> selected() const { return selected_; }
    void set_selected(std::optional<calendar::Date> date);
    calendar::YearMonth displayed_month() const { return displayed_; }
    void set_displayed_month(calendar::YearMonth month, bool animate = true);
    DatePickerMode mode() const { return mode_; }
    void set_mode(DatePickerMode mode);
    bool year_picker_open() const { return years_open_; }
    void set_year_picker_open(bool open);
    const calendar::CalendarLocale& locale() const { return *locale_; }
    void set_locale(const calendar::CalendarLocale& locale);
    void set_today(calendar::Date today) { today_ = today; }
    std::optional<calendar::Date> focused_day() const;
    std::optional<int> focused_year() const;
    std::string headline() const;
    std::string title() const;
    const std::string& input_error() const { return input_error_; }
    std::string input_text() const;
    const TextField& input_field() const { return *input_; }
    double month_slide() const { return slide_.value(); }
    double year_presence() const { return year_expand_.value(); }
    double mode_progress() const { return mode_progress_.value(); }
    render::Rect day_bounds(const calendar::Date& date) const;
    render::Rect year_bounds(int year) const;
    render::Rect content_top_left() const;

    const IconButton& mode_toggle() const { return toggle_; }
    const IconButton& previous_button() const { return previous_button_; }
    const IconButton& next_button() const { return next_button_; }
    bool header_shown() const { return options_.show_header && options_.show_mode_toggle; }
    bool navigation_shown() const { return days_shown(); }
    void add_focus_stops(FocusManager& focus);
    bool focused() const;
    void clear_focus();
    void focus_grid();
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
    struct MenuButton {
        PressState press;
        motion::Animated state_layer;
        FocusIndicator focus;
    };
    struct Pointer {
        std::optional<calendar::Date> hovered_day;
        std::optional<calendar::Date> pressed_day;
        std::optional<int> hovered_year;
        std::optional<int> pressed_year;
        motion::Animated state_layer;
    };

    double header_height() const;
    double content_top() const;
    double picker_height() const;
    double input_height() const;
    double content_height(DatePickerMode mode) const;
    render::Point grid_origin() const;
    render::Point weekday_origin() const;
    render::Rect years_area() const;
    render::Rect menu_button_rect() const;
    std::string month_label() const;
    MonthGridState grid_state(bool with_pointer) const;
    void select(const calendar::Date& date, DatePickerResponse& response);
    void focus_day(const calendar::Date& date);
    void pick_year(int year);
    void rebuild_input();
    void sync_input_from_selection();
    void on_input_changed(DatePickerResponse& response);
    void place_children();
    void layout_years();
    bool picker_shown() const { return mode_ == DatePickerMode::picker; }
    bool days_shown() const { return picker_shown() && !years_open_; }
    void paint_header(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_picker(render::Canvas& canvas, const color::ColorScheme& scheme, double dy) const;
    void paint_years(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_input(render::Canvas& canvas, const color::ColorScheme& scheme, double dy) const;

    DatePickerOptions options_;
    text::FontStack fonts_;
    icons::IconFont icon_font_;
    const calendar::CalendarLocale* locale_;
    calendar::Date today_;
    std::optional<calendar::Date> selected_;
    calendar::YearMonth displayed_;
    calendar::YearMonth previous_;
    int slide_direction_ = 0;
    DatePickerMode mode_;
    bool years_open_ = false;
    render::Point origin_{0.0, 0.0};

    IconButton toggle_;
    IconButton previous_button_;
    IconButton next_button_;
    MenuButton menu_;
    std::unique_ptr<TextField> input_;
    std::string input_error_;
    double input_focus_delay_ = -1.0;

    FocusIndicator grid_focus_;
    calendar::Date focus_date_;
    FocusIndicator year_focus_;
    int focus_year_ = 0;
    Pointer pointer_;
    ScrollArea years_scroll_;
    std::vector<std::unique_ptr<FocusStop>> stops_;

    motion::Animated slide_;
    motion::Animated year_expand_;
    motion::Animated year_alpha_;
    motion::Animated mode_progress_;
    motion::Animated incoming_alpha_;
    motion::Animated outgoing_alpha_;
    motion::Animated height_;
    std::optional<calendar::Date> fading_;
    motion::Animated selection_fade_;
    Enablement enabled_;
};

}
