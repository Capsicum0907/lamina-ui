#pragma once

#include <memory>
#include <optional>
#include <string>

#include "lamina/calendar/locale.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/widgets/date_picker.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/popup_transition.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

struct DockedDatePickerOptions {
    calendar::Date today;
    std::optional<calendar::Date> selected = std::nullopt;
    calendar::YearRange year_range{1900, 2100};
    SelectableDates selectable = {};
    const calendar::CalendarLocale* locale = nullptr;
    input::Clipboard* clipboard = nullptr;
    bool enabled = true;
    std::string open_name = {};
};

class DockedDatePicker {
public:
    DockedDatePicker(DockedDatePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);
    DockedDatePicker(const DockedDatePicker&) = delete;
    DockedDatePicker& operator=(const DockedDatePicker&) = delete;

    double width() const;
    double height() const;
    void set_origin(render::Point origin);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;
    render::Rect popup_bounds() const;

    std::optional<calendar::Date> selected() const { return selected_; }
    void set_selected(std::optional<calendar::Date> date);
    bool is_open() const { return popup_.is_open(); }
    void open(input::InputSource source = input::InputSource::pointer);
    void close();
    void set_locale(const calendar::CalendarLocale& locale);
    const TextField& field() const { return *field_; }
    DatePicker& picker() { return picker_; }
    const IconButton& open_button() const { return open_button_; }

    void add_focus_stops(FocusManager& focus);
    bool focused() const;
    bool enabled() const { return enabled_; }
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
    void paint_popup(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void rebuild_field();
    void sync_field();
    void on_field_changed(DatePickerResponse& response);
    void place();

    DockedDatePickerOptions options_;
    text::FontStack fonts_;
    icons::IconFont icon_font_;
    const calendar::CalendarLocale* locale_;
    std::optional<calendar::Date> selected_;
    std::unique_ptr<TextField> field_;
    IconButton open_button_;
    DatePicker picker_;
    PopupTransition popup_;
    std::string error_;
    render::Point origin_{0.0, 0.0};
    bool enabled_;
    std::unique_ptr<FocusStop> field_stop_;
};

}
