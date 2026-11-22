#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/calendar/locale.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/time_picker.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

enum class TimePickerLayout { vertical, horizontal };
enum class TimePickerMode { dial, input };
enum class TimeSelection { hour, minute };

struct TimePickerOptions {
    int hour = 0;
    int minute = 0;
    std::optional<bool> is_24_hour = std::nullopt;
    TimePickerLayout layout = TimePickerLayout::vertical;
    TimePickerMode mode = TimePickerMode::dial;
    const calendar::CalendarLocale* locale = nullptr;
    bool enabled = true;
};

struct TimePickerResponse {
    bool changed = false;
    bool consumed = false;
};

struct DialLabel {
    int value;
    bool inner;
    render::Point center;
};

class TimePicker {
public:
    TimePicker(TimePickerOptions options, const text::FontStack& fonts);
    TimePicker(const TimePicker&) = delete;
    TimePicker& operator=(const TimePicker&) = delete;

    double width() const;
    double height() const;
    void set_origin(render::Point origin);
    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& r);
    render::Rect bounds() const;

    int hour() const { return hour_; }
    int minute() const { return minute_; }
    void set_time(int hour, int minute);
    bool is_24_hour() const { return is_24_hour_; }
    void set_24_hour(bool on);
    bool is_pm() const { return hour_ >= 12; }
    int display_hour() const;
    TimeSelection selection() const { return selection_; }
    void set_selection(TimeSelection selection);
    TimePickerMode mode() const { return mode_; }
    void set_mode(TimePickerMode mode);
    TimePickerLayout layout() const { return layout_; }
    void set_layout(TimePickerLayout layout);
    const calendar::CalendarLocale& locale() const { return *locale_; }
    void set_locale(const calendar::CalendarLocale& locale) { locale_ = &locale; }

    double selector_angle() const { return angle_.value(); }
    double numbers_progress() const { return numbers_.value(); }
    render::Rect hour_selector_bounds() const;
    render::Rect minute_selector_bounds() const;
    render::Rect period_bounds(bool pm) const;
    render::Point dial_center() const;
    double dial_size() const { return tokens::time_picker_tokens.dial_size; }
    std::vector<DialLabel> dial_labels() const;
    std::optional<render::Point> label_center(int value) const;
    const std::string& hour_text() const { return fields_[0].text; }
    const std::string& minute_text() const { return fields_[1].text; }
    std::string hour_error() const;
    std::string minute_error() const;
    std::string formatted() const;

    void add_focus_stops(FocusManager& focus);
    bool focused() const;
    void clear_focus();
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    TimePickerResponse handle(const input::PointerEvent& event);
    bool holds_pointer() const { return dragging_; }
    TimePickerResponse handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent&) { return false; }
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Toggle {
        PressState press;
        motion::Animated state_layer;
        motion::Animated corner;
        motion::Animated checked;
    };
    struct Field {
        std::string text;
        FocusIndicator focus;
        bool focused = false;
    };

    double display_width() const;
    double display_height() const;
    render::Point display_origin() const;
    render::Rect field_bounds(int index) const;
    double dial_angle_of(int value, TimeSelection selection) const;
    double target_angle() const;
    void animate_selector();
    void choose(double x, double y, bool snap_minutes);
    void set_hour_value(int hour, TimePickerResponse& response);
    void set_minute_value(int minute, TimePickerResponse& response);
    void set_period(bool pm, TimePickerResponse& response);
    void type_digit(int field, const std::string& digits, TimePickerResponse& response);
    void apply_field(int field, TimePickerResponse& response);
    void sync_fields();
    void focus_field(int field);
    void paint_display(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_period(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_dial(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_inputs(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_labels(render::Canvas& canvas, TimeSelection selection, double alpha, color::Argb color) const;
    std::vector<DialLabel> labels_for(TimeSelection selection) const;
    bool hour_valid() const;
    bool minute_valid() const;

    TimePickerOptions options_;
    text::FontStack fonts_;
    const calendar::CalendarLocale* locale_;
    int hour_;
    int minute_;
    bool is_24_hour_;
    TimeSelection selection_ = TimeSelection::hour;
    TimeSelection previous_selection_ = TimeSelection::hour;
    TimePickerMode mode_;
    TimePickerLayout layout_;
    render::Point origin_{0.0, 0.0};

    motion::Animated angle_;
    motion::Animated numbers_;
    motion::Animated mode_alpha_;
    std::array<Toggle, 2> periods_;
    std::array<Toggle, 2> selectors_;
    std::array<FocusIndicator, 2> selector_focus_;
    FocusIndicator period_focus_;
    FocusIndicator dial_focus_;
    std::size_t dial_focus_index_ = 0;
    std::array<Field, 2> fields_;
    int hour_input_;
    int minute_input_;
    double blink_clock_ = 0.0;
    bool dragging_ = false;
    bool moved_ = false;
    render::Point press_{0.0, 0.0};
    double switch_delay_ = -1.0;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    Enablement enabled_;
};

}
