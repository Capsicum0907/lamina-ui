#include "lamina/widgets/time_picker.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::TimePickerTokens& t = tokens::time_picker_tokens;
constexpr const tokens::TimePickerColors& k = tokens::time_picker_colors;
constexpr const tokens::TimePickerMotion& m = tokens::time_picker_motion;
constexpr int labels_per_ring = 12;
constexpr double full_turn = 360.0;
constexpr double hour_step = full_turn / labels_per_ring;
constexpr double minute_step = full_turn / 60.0;

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

color::Argb dim(color::Argb c, double enabled) {
    return color::mix(color::scale_alpha(c, k.disabled_opacity), c, enabled);
}

double normalized(double degrees) {
    double d = std::fmod(degrees, full_turn);
    if (d <= -full_turn / 2.0) {
        d += full_turn;
    }
    if (d > full_turn / 2.0) {
        d -= full_turn;
    }
    return d;
}

render::Point on_circle(render::Point c, double radius, double degrees) {
    const double r = degrees * std::numbers::pi / 180.0;
    return {c.x + radius * std::sin(r), c.y - radius * std::cos(r)};
}

std::string two_digits(int v) {
    return calendar::format_number(v, 2);
}

}

TimePicker::TimePicker(TimePickerOptions options, const text::FontStack& fonts)
    : options_(options),
      fonts_(fonts),
      locale_(options.locale ? options.locale : &calendar::english_us()),
      hour_(std::clamp(options.hour, 0, 23)),
      minute_(std::clamp(options.minute, 0, 59)),
      is_24_hour_(options.is_24_hour.value_or(locale_->uses_24_hour_clock)),
      mode_(options.mode),
      layout_(options.layout),
      angle_(0.0, m.selector),
      numbers_(1.0, m.numbers),
      mode_alpha_(1.0, m.mode),
      periods_{Toggle{{}, motion::Animated(0.0, m.state_layer), motion::Animated(0.0, m.period_shape), motion::Animated(0.0, m.colors)},
               Toggle{{}, motion::Animated(0.0, m.state_layer), motion::Animated(0.0, m.period_shape), motion::Animated(0.0, m.colors)}},
      selectors_{Toggle{{}, motion::Animated(0.0, m.state_layer), motion::Animated(0.0, m.period_shape), motion::Animated(1.0, m.colors)},
                 Toggle{{}, motion::Animated(0.0, m.state_layer), motion::Animated(0.0, m.period_shape), motion::Animated(0.0, m.colors)}},
      hour_input_(hour_),
      minute_input_(minute_),
      enabled_(options.enabled) {
    angle_.snap_to(target_angle());
    const double pm = is_pm() ? 1.0 : 0.0;
    periods_[0].checked.snap_to(1.0 - pm);
    periods_[1].checked.snap_to(pm);
    sync_fields();
}

int TimePicker::display_hour() const {
    if (is_24_hour_) {
        return hour_;
    }
    return hour_ % 12 == 0 ? 12 : hour_ % 12;
}

std::string TimePicker::formatted() const {
    return calendar::format_time(is_24_hour_ ? locale_->time_24_pattern : locale_->time_12_pattern, hour_, minute_, locale_->names);
}

double TimePicker::display_width() const {
    const double base = 2.0 * t.selector_width + t.separator_width;
    if (mode_ == TimePickerMode::dial && layout_ == TimePickerLayout::horizontal) {
        return base;
    }
    return base + (is_24_hour_ ? 0.0 : t.period_start_padding + t.period_vertical_width);
}

double TimePicker::display_height() const {
    if (mode_ == TimePickerMode::input) {
        return t.input_field_height;
    }
    if (layout_ == TimePickerLayout::horizontal && !is_24_hour_) {
        return t.selector_height + t.period_top_padding + t.period_horizontal_height;
    }
    return t.selector_height;
}

double TimePicker::width() const {
    if (mode_ == TimePickerMode::input) {
        return display_width();
    }
    if (layout_ == TimePickerLayout::horizontal) {
        return display_width() + t.display_bottom + t.dial_size;
    }
    return std::max(display_width(), t.dial_size);
}

double TimePicker::height() const {
    if (mode_ == TimePickerMode::input) {
        return t.input_field_height + t.input_supporting_top + t.input_supporting_lines * style_of(t.input_supporting_type).line_height;
    }
    if (layout_ == TimePickerLayout::horizontal) {
        return std::max(display_height(), t.dial_size);
    }
    return t.selector_height + t.display_bottom + t.dial_size + t.dial_bottom;
}

void TimePicker::set_origin(render::Point origin) {
    origin_ = origin;
}

layout::Size TimePicker::measure(const layout::Constraints&) const {
    return {width(), height()};
}

void TimePicker::arrange(const render::Rect& r) {
    origin_ = {r.x, r.y};
}

render::Rect TimePicker::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

render::Point TimePicker::display_origin() const {
    if (mode_ == TimePickerMode::input) {
        return origin_;
    }
    if (layout_ == TimePickerLayout::horizontal) {
        return {origin_.x, origin_.y + (height() - display_height()) / 2.0};
    }
    return {origin_.x + (width() - display_width()) / 2.0, origin_.y};
}

render::Rect TimePicker::field_bounds(int index) const {
    const render::Point o = display_origin();
    const double h = mode_ == TimePickerMode::input ? t.input_field_height : t.selector_height;
    return {o.x + index * (t.selector_width + t.separator_width), o.y, t.selector_width, h};
}

render::Rect TimePicker::hour_selector_bounds() const {
    return field_bounds(0);
}

render::Rect TimePicker::minute_selector_bounds() const {
    return field_bounds(1);
}

render::Rect TimePicker::period_bounds(bool pm) const {
    const render::Point o = display_origin();
    if (mode_ == TimePickerMode::dial && layout_ == TimePickerLayout::horizontal) {
        const double w = (t.period_horizontal_width - t.period_gap) / 2.0;
        const double y = o.y + t.selector_height + t.period_top_padding;
        return {o.x + (pm ? w + t.period_gap : 0.0), y, w, t.period_horizontal_height};
    }
    const double total = mode_ == TimePickerMode::input ? t.input_field_height : t.period_vertical_height;
    const double h = (total - t.period_gap) / 2.0;
    const double x = o.x + 2.0 * t.selector_width + t.separator_width + t.period_start_padding;
    return {x, o.y + (pm ? h + t.period_gap : 0.0), t.period_vertical_width, h};
}

render::Point TimePicker::dial_center() const {
    if (layout_ == TimePickerLayout::horizontal) {
        return {origin_.x + display_width() + t.display_bottom + t.dial_size / 2.0, origin_.y + height() / 2.0};
    }
    return {origin_.x + width() / 2.0, origin_.y + t.selector_height + t.display_bottom + t.dial_size / 2.0};
}

std::vector<DialLabel> TimePicker::dial_labels() const {
    return labels_for(selection_);
}

std::vector<DialLabel> TimePicker::labels_for(TimeSelection selection) const {
    std::vector<DialLabel> out;
    const render::Point c = dial_center();
    const double outer = t.dial_size * t.outer_ratio;
    const double inner = t.dial_size * t.inner_ratio;
    for (int i = 0; i < labels_per_ring; ++i) {
        const double a = i * hour_step;
        if (selection == TimeSelection::minute) {
            out.push_back({i * 5, false, on_circle(c, outer, a)});
        } else if (is_24_hour_) {
            out.push_back({i, false, on_circle(c, outer, a)});
        } else {
            out.push_back({i == 0 ? 12 : i, false, on_circle(c, outer, a)});
        }
    }
    if (selection == TimeSelection::hour && is_24_hour_) {
        for (int i = 0; i < labels_per_ring; ++i) {
            out.push_back({i + 12, true, on_circle(c, inner, i * hour_step)});
        }
    }
    return out;
}

std::optional<render::Point> TimePicker::label_center(int value) const {
    for (const DialLabel& l : dial_labels()) {
        if (l.value == value) {
            return l.center;
        }
    }
    return std::nullopt;
}

double TimePicker::dial_angle_of(int value, TimeSelection selection) const {
    return selection == TimeSelection::minute ? value * minute_step : (value % labels_per_ring) * hour_step;
}

double TimePicker::target_angle() const {
    return dial_angle_of(selection_ == TimeSelection::minute ? minute_ : hour_, selection_);
}

void TimePicker::animate_selector() {
    const double c = angle_.value();
    angle_.animate_to(c + normalized(target_angle() - c));
}

void TimePicker::set_time(int hour, int minute) {
    hour_ = std::clamp(hour, 0, 23);
    minute_ = std::clamp(minute, 0, 59);
    hour_input_ = hour_;
    minute_input_ = minute_;
    angle_.snap_to(target_angle());
    periods_[0].checked.animate_to(is_pm() ? 0.0 : 1.0);
    periods_[1].checked.animate_to(is_pm() ? 1.0 : 0.0);
    sync_fields();
}

void TimePicker::set_24_hour(bool on) {
    is_24_hour_ = on;
    sync_fields();
}

void TimePicker::set_selection(TimeSelection selection) {
    if (selection == selection_) {
        return;
    }
    previous_selection_ = selection_;
    selection_ = selection;
    numbers_.snap_to(0.0);
    numbers_.animate_to(1.0);
    selectors_[0].checked.animate_to(selection == TimeSelection::hour ? 1.0 : 0.0);
    selectors_[1].checked.animate_to(selection == TimeSelection::minute ? 1.0 : 0.0);
    animate_selector();
    dial_focus_index_ = 0;
    switch_delay_ = -1.0;
}

void TimePicker::set_mode(TimePickerMode mode) {
    if (mode == mode_) {
        return;
    }
    clear_focus();
    mode_ = mode;
    mode_alpha_.snap_to(0.0);
    mode_alpha_.animate_to(1.0);
    sync_fields();
}

void TimePicker::set_layout(TimePickerLayout layout) {
    layout_ = layout;
}

bool TimePicker::hour_valid() const {
    return hour_input_ >= 0 && hour_input_ <= 23;
}

bool TimePicker::minute_valid() const {
    return minute_input_ >= 0 && minute_input_ <= 59;
}

std::string TimePicker::hour_error() const {
    return hour_valid() ? std::string() : is_24_hour_ ? locale_->strings.hour_error_24h : locale_->strings.hour_error;
}

std::string TimePicker::minute_error() const {
    return minute_valid() ? std::string() : locale_->strings.minute_error;
}

void TimePicker::sync_fields() {
    fields_[0].text = two_digits(display_hour());
    fields_[1].text = two_digits(minute_);
    hour_input_ = hour_;
    minute_input_ = minute_;
}

void TimePicker::set_hour_value(int hour, TimePickerResponse& response) {
    if (hour != hour_) {
        hour_ = hour;
        response.changed = true;
    }
    hour_input_ = hour_;
    periods_[0].checked.animate_to(is_pm() ? 0.0 : 1.0);
    periods_[1].checked.animate_to(is_pm() ? 1.0 : 0.0);
    fields_[0].text = two_digits(display_hour());
}

void TimePicker::set_minute_value(int minute, TimePickerResponse& response) {
    if (minute != minute_) {
        minute_ = minute;
        response.changed = true;
    }
    minute_input_ = minute_;
    fields_[1].text = two_digits(minute_);
}

void TimePicker::set_period(bool pm, TimePickerResponse& response) {
    if (pm == is_pm() || !hour_valid()) {
        return;
    }
    set_hour_value(pm ? hour_ + 12 : hour_ - 12, response);
}

void TimePicker::choose(double x, double y, bool tap) {
    const render::Point c = dial_center();
    const double dx = x - c.x;
    const double dy = y - c.y;
    double theta = std::atan2(dx, -dy) * 180.0 / std::numbers::pi;
    if (theta < 0.0) {
        theta += full_turn;
    }
    TimePickerResponse ignored;
    if (selection_ == TimeSelection::minute) {
        const int minute = tap ? static_cast<int>(std::lround(theta / hour_step)) * t.minute_tap_snap % 60
                               : static_cast<int>(std::lround(theta / minute_step)) % 60;
        set_minute_value(minute, ignored);
    } else {
        const int h12 = static_cast<int>(std::lround(theta / hour_step)) % labels_per_ring;
        int hour = h12;
        if (is_24_hour_) {
            const double scale = t.dial_size / tokens::time_picker_tokens.dial_size;
            const bool inner = std::hypot(dx, dy) < t.inner_ring_distance * scale;
            hour = inner ? h12 + 12 : h12;
        } else {
            hour = h12 + (is_pm() ? 12 : 0);
        }
        set_hour_value(hour, ignored);
    }
    if (tap) {
        animate_selector();
    } else {
        const double a = angle_.value();
        angle_.snap_to(a + normalized(theta - a));
    }
}


void TimePicker::focus_field(int field) {
    for (int i = 0; i < 2; ++i) {
        fields_[i].focused = i == field;
    }
    blink_clock_ = 0.0;
    set_selection(field == 0 ? TimeSelection::hour : TimeSelection::minute);
}

void TimePicker::apply_field(int field, TimePickerResponse& response) {
    const std::string& text = fields_[field].text;
    const int n = text.empty() ? -1 : std::stoi(text);
    if (field == 1) {
        minute_input_ = n < 0 ? 0 : n;
        if (minute_valid() && minute_input_ != minute_) {
            minute_ = minute_input_;
            response.changed = true;
        }
        return;
    }
    if (is_24_hour_) {
        hour_input_ = n < 0 ? 0 : n;
    } else if (n < 0) {
        hour_input_ = is_pm() ? 12 : 0;
    } else if (n == 12) {
        hour_input_ = is_pm() ? 12 : 0;
    } else if (n >= 1 && n <= 11) {
        hour_input_ = is_pm() ? n + 12 : n;
    } else {
        hour_input_ = -1;
    }
    if (hour_valid() && hour_input_ != hour_) {
        hour_ = hour_input_;
        response.changed = true;
    }
}

void TimePicker::type_digit(int field, const std::string& digits, TimePickerResponse& response) {
    for (char d : digits) {
        std::string& text = fields_[field].text;
        if (text.size() >= 2) {
            text = std::string(1, d);
        } else {
            text.push_back(d);
        }
    }
    blink_clock_ = 0.0;
    apply_field(field, response);
    if (field == 0 && hour_valid()) {
        const std::string& text = fields_[0].text;
        const bool single = !is_24_hour_ && text.size() == 1 && text[0] >= '2' && text[0] <= '9';
        if (text.size() == 2 || single) {
            focus_field(1);
        }
    }
}

void TimePicker::add_focus_stops(FocusManager& focus) {
    auto add = [&](FocusStop::Hooks hooks) {
        stops_.push_back(std::make_unique<FocusStop>(std::move(hooks)));
        focus.add(*stops_.back());
    };
    for (int i = 0; i < 2; ++i) {
        add({[this, i] { return mode_ == TimePickerMode::dial ? selector_focus_[i].focused() : fields_[i].focused; },
             [this, i](bool f) {
                 f = f && enabled();
                 if (mode_ == TimePickerMode::dial) {
                     selector_focus_[i].set_focused(f);
                 } else if (f) {
                     focus_field(i);
                 } else {
                     fields_[i].focused = false;
                 }
             },
             [this] { return enabled(); }, [this, i] { return field_bounds(i); }});
    }
    add({[this] { return period_focus_.focused(); }, [this](bool f) { period_focus_.set_focused(f && enabled()); },
         [this] { return enabled() && !is_24_hour_; }, [this] { return period_bounds(is_pm()); }});
    add({[this] { return dial_focus_.focused(); },
         [this](bool f) {
             if (f && !dial_focus_.focused()) {
                 const std::vector<DialLabel> labels = dial_labels();
                 const int value = selection_ == TimeSelection::minute ? minute_ : (is_24_hour_ ? hour_ : display_hour());
                 dial_focus_index_ = 0;
                 int best = 1000;
                 for (std::size_t i = 0; i < labels.size(); ++i) {
                     const int d = std::abs(labels[i].value - value);
                     if (d < best) {
                         best = d;
                         dial_focus_index_ = i;
                     }
                 }
             }
             dial_focus_.set_focused(f && enabled() && mode_ == TimePickerMode::dial);
         },
         [this] { return enabled() && mode_ == TimePickerMode::dial; },
         [this] {
             const std::vector<DialLabel> labels = dial_labels();
             const render::Point p = labels[std::min(dial_focus_index_, labels.size() - 1)].center;
             return render::Rect{p.x - t.label_size / 2.0, p.y - t.label_size / 2.0, t.label_size, t.label_size};
         }});
}

bool TimePicker::focused() const {
    return selector_focus_[0].focused() || selector_focus_[1].focused() || period_focus_.focused() || dial_focus_.focused() ||
           fields_[0].focused || fields_[1].focused;
}

void TimePicker::clear_focus() {
    selector_focus_[0].set_focused(false);
    selector_focus_[1].set_focused(false);
    period_focus_.set_focused(false);
    dial_focus_.set_focused(false);
    fields_[0].focused = false;
    fields_[1].focused = false;
}

void TimePicker::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        clear_focus();
        cancel_interaction();
    }
}

AccessibleNode TimePicker::accessible() const {
    const calendar::PickerStrings& s = locale_->strings;
    AccessibleNode node{AccessibleRole::group, formatted(), bounds(), {.enabled = enabled()}};
    if (mode_ == TimePickerMode::dial) {
        node.children.push_back({AccessibleRole::radio_button, s.hour_selection, field_bounds(0),
                                 {.enabled = enabled(), .focusable = enabled(), .focused = selector_focus_[0].focused(),
                                  .selected = selection_ == TimeSelection::hour},
                                 two_digits(display_hour())});
        node.children.push_back({AccessibleRole::radio_button, s.minute_selection, field_bounds(1),
                                 {.enabled = enabled(), .focusable = enabled(), .focused = selector_focus_[1].focused(),
                                  .selected = selection_ == TimeSelection::minute},
                                 two_digits(minute_)});
    } else {
        node.children.push_back({AccessibleRole::text_field, s.hour_text_field, field_bounds(0),
                                 {.enabled = enabled(), .focusable = enabled(), .focused = fields_[0].focused, .invalid = !hour_valid()},
                                 fields_[0].text, hour_valid() ? s.hour : hour_error()});
        node.children.push_back({AccessibleRole::text_field, s.minute_text_field, field_bounds(1),
                                 {.enabled = enabled(), .focusable = enabled(), .focused = fields_[1].focused, .invalid = !minute_valid()},
                                 fields_[1].text, minute_valid() ? s.minute : minute_error()});
    }
    if (!is_24_hour_) {
        AccessibleNode period{AccessibleRole::group, s.period_toggle_description, period_bounds(false),
                              {.enabled = enabled(), .focusable = enabled(), .focused = period_focus_.focused()}};
        period.children.push_back({AccessibleRole::checkbox, s.am, period_bounds(false),
                                   {.enabled = enabled(), .toggle = is_pm() ? AccessibleToggle::off : AccessibleToggle::on, .selected = !is_pm()}});
        period.children.push_back({AccessibleRole::checkbox, s.pm, period_bounds(true),
                                   {.enabled = enabled(), .toggle = is_pm() ? AccessibleToggle::on : AccessibleToggle::off, .selected = is_pm()}});
        node.children.push_back(std::move(period));
    }
    if (mode_ == TimePickerMode::dial) {
        const render::Point c = dial_center();
        AccessibleNode dial{AccessibleRole::group, selection_ == TimeSelection::hour ? s.hour_selection : s.minute_selection,
                            {c.x - t.dial_size / 2.0, c.y - t.dial_size / 2.0, t.dial_size, t.dial_size},
                            {.enabled = enabled(), .focusable = enabled(), .focused = dial_focus_.focused()}};
        const std::vector<DialLabel> labels = dial_labels();
        const int current = selection_ == TimeSelection::minute ? minute_ : (is_24_hour_ ? hour_ : display_hour());
        for (std::size_t i = 0; i < labels.size(); ++i) {
            const DialLabel& l = labels[i];
            const std::string& tmpl = selection_ == TimeSelection::minute ? s.minute_suffix : is_24_hour_ ? s.hour_24h_suffix : s.hour_suffix;
            dial.children.push_back({AccessibleRole::button, calendar::substitute(tmpl, calendar::format_number(l.value)),
                                     {l.center.x - t.label_size / 2.0, l.center.y - t.label_size / 2.0, t.label_size, t.label_size},
                                     {.enabled = enabled(), .focused = dial_focus_.focused() && dial_focus_index_ == i, .selected = l.value == current}});
        }
        node.children.push_back(std::move(dial));
    }
    return node;
}

TimePickerResponse TimePicker::handle(const input::PointerEvent& e) {
    TimePickerResponse response;
    for (FocusIndicator* f : {&selector_focus_[0], &selector_focus_[1], &period_focus_, &dial_focus_}) {
        f->release_on_press(e);
    }
    if (!enabled()) {
        return response;
    }
    for (int i = 0; i < 2; ++i) {
        if (selectors_[i].press.update(e, field_bounds(i))) {
            if (mode_ == TimePickerMode::dial) {
                set_selection(i == 0 ? TimeSelection::hour : TimeSelection::minute);
            }
            response.consumed = true;
        }
        if (mode_ == TimePickerMode::input && e.action == input::PointerAction::down && contains(field_bounds(i), e.x_dp, e.y_dp)) {
            focus_field(i);
            response.consumed = true;
        }
        selectors_[i].state_layer.animate_to(mode_ == TimePickerMode::dial ? selectors_[i].press.state_layer_opacity() : 0.0);
    }
    if (mode_ == TimePickerMode::input && e.action == input::PointerAction::down && !response.consumed && !contains(bounds(), e.x_dp, e.y_dp)) {
        fields_[0].focused = false;
        fields_[1].focused = false;
    }
    if (!is_24_hour_) {
        for (int i = 0; i < 2; ++i) {
            if (periods_[i].press.update(e, period_bounds(i == 1))) {
                set_period(i == 1, response);
                response.consumed = true;
            }
            periods_[i].state_layer.animate_to(periods_[i].press.state_layer_opacity());
            periods_[i].corner.animate_to(periods_[i].press.pressed ? 1.0 : 0.0);
        }
    }
    if (mode_ != TimePickerMode::dial) {
        return response;
    }
    const render::Point c = dial_center();
    const bool inside = std::hypot(e.x_dp - c.x, e.y_dp - c.y) <= t.dial_size / 2.0;
    switch (e.action) {
    case input::PointerAction::down:
        if (inside) {
            dragging_ = true;
            moved_ = false;
            press_ = {e.x_dp, e.y_dp};
            switch_delay_ = -1.0;
            response.consumed = true;
        }
        break;
    case input::PointerAction::move:
        if (dragging_) {
            if (!moved_ && std::hypot(e.x_dp - press_.x, e.y_dp - press_.y) > t.touch_slop) {
                moved_ = true;
            }
            if (moved_) {
                const int before_h = hour_;
                const int before_m = minute_;
                choose(e.x_dp, e.y_dp, false);
                response.changed = before_h != hour_ || before_m != minute_;
            }
            response.consumed = true;
        }
        break;
    case input::PointerAction::up:
        if (dragging_) {
            const int before_h = hour_;
            const int before_m = minute_;
            if (!moved_) {
                choose(e.x_dp, e.y_dp, true);
                if (selection_ == TimeSelection::hour) {
                    switch_delay_ = t.auto_switch_delay_seconds;
                }
            } else {
                choose(e.x_dp, e.y_dp, false);
                if (selection_ == TimeSelection::hour) {
                    set_selection(TimeSelection::minute);
                } else {
                    animate_selector();
                }
            }
            response.changed = before_h != hour_ || before_m != minute_;
            response.consumed = true;
        }
        dragging_ = false;
        break;
    case input::PointerAction::leave:
        if (dragging_) {
            animate_selector();
        }
        dragging_ = false;
        break;
    }
    return response;
}

TimePickerResponse TimePicker::handle(const input::KeyEvent& e) {
    TimePickerResponse response;
    if (!enabled() || e.ctrl) {
        return response;
    }
    for (int i = 0; i < 2; ++i) {
        if (selector_focus_[i].activates(e, KeyActivation::space_or_enter)) {
            set_selection(i == 0 ? TimeSelection::hour : TimeSelection::minute);
            response.consumed = true;
            return response;
        }
        if (fields_[i].focused) {
            if (e.key == input::Key::backspace && !fields_[i].text.empty()) {
                fields_[i].text.pop_back();
                apply_field(i, response);
                response.consumed = true;
            } else if (e.key == input::Key::enter && i == 0 && hour_valid()) {
                focus_field(1);
                response.consumed = true;
            }
            return response;
        }
    }
    if (period_focus_.focused()) {
        switch (e.key) {
        case input::Key::left:
        case input::Key::up: set_period(false, response); break;
        case input::Key::right:
        case input::Key::down: set_period(true, response); break;
        default:
            if (period_focus_.activates(e, KeyActivation::space_or_enter)) {
                set_period(!is_pm(), response);
            } else {
                return response;
            }
        }
        response.consumed = true;
        return response;
    }
    if (dial_focus_.focused()) {
        const std::vector<DialLabel> labels = dial_labels();
        const std::size_t n = labels.size();
        switch (e.key) {
        case input::Key::right:
        case input::Key::down: dial_focus_index_ = (dial_focus_index_ + 1) % n; break;
        case input::Key::left:
        case input::Key::up: dial_focus_index_ = (dial_focus_index_ + n - 1) % n; break;
        default:
            if (dial_focus_.activates(e, KeyActivation::space_or_enter)) {
                const DialLabel& l = labels[dial_focus_index_ % n];
                if (selection_ == TimeSelection::minute) {
                    set_minute_value(l.value, response);
                } else if (is_24_hour_) {
                    set_hour_value(l.value, response);
                } else {
                    set_hour_value(l.value % 12 + (is_pm() ? 12 : 0), response);
                }
                angle_.snap_to(angle_.value() + normalized(target_angle() - angle_.value()));
            } else {
                return response;
            }
        }
        response.consumed = true;
    }
    return response;
}

void TimePicker::handle(const input::TextEvent& e) {
    if (!enabled() || e.kind != input::TextEventKind::commit) {
        return;
    }
    for (int i = 0; i < 2; ++i) {
        if (!fields_[i].focused) {
            continue;
        }
        std::string digits;
        for (char32_t c : text::decode_utf8(e.text)) {
            if (const std::optional<char32_t> d = ascii_digit(c)) {
                digits.push_back(static_cast<char>(*d));
            }
        }
        if (!digits.empty()) {
            TimePickerResponse ignored;
            type_digit(i, digits, ignored);
        }
        return;
    }
}

void TimePicker::cancel_interaction() {
    for (Toggle& toggle : selectors_) {
        toggle.press = {};
        toggle.state_layer.animate_to(0.0);
    }
    for (Toggle& toggle : periods_) {
        toggle.press = {};
        toggle.state_layer.animate_to(0.0);
        toggle.corner.animate_to(0.0);
    }
    if (dragging_) {
        animate_selector();
    }
    dragging_ = false;
}

void TimePicker::step(double dt) {
    angle_.step(dt);
    numbers_.step(dt);
    mode_alpha_.step(dt);
    for (Toggle& toggle : selectors_) {
        toggle.state_layer.step(dt);
        toggle.corner.step(dt);
        toggle.checked.step(dt);
    }
    for (Toggle& toggle : periods_) {
        toggle.state_layer.step(dt);
        toggle.corner.step(dt);
        toggle.checked.step(dt);
    }
    for (FocusIndicator& f : selector_focus_) {
        f.step(dt);
    }
    period_focus_.step(dt);
    dial_focus_.step(dt);
    enabled_.step(dt);
    blink_clock_ += dt;
    if (switch_delay_ >= 0.0 && angle_.settled()) {
        switch_delay_ -= dt;
        if (switch_delay_ < 0.0) {
            set_selection(TimeSelection::minute);
        }
    }
}

bool TimePicker::animating() const {
    auto toggles = [](const std::array<Toggle, 2>& a) {
        return std::any_of(a.begin(), a.end(), [](const Toggle& x) { return !x.state_layer.settled() || !x.corner.settled() || !x.checked.settled(); });
    };
    return !angle_.settled() || !numbers_.settled() || !mode_alpha_.settled() || toggles(selectors_) || toggles(periods_) ||
           !selector_focus_[0].settled() || !selector_focus_[1].settled() || !period_focus_.settled() || !dial_focus_.settled() ||
           !enabled_.settled() || switch_delay_ >= 0.0;
}

std::optional<render::Rect> TimePicker::caret_area() const {
    for (int i = 0; i < 2; ++i) {
        if (fields_[i].focused && mode_ == TimePickerMode::input) {
            return field_bounds(i);
        }
    }
    return std::nullopt;
}

std::optional<double> TimePicker::next_wake_seconds() const {
    if (mode_ == TimePickerMode::input && (fields_[0].focused || fields_[1].focused)) {
        const double blink = tokens::outlined_text_field.caret_blink_seconds;
        return blink - std::fmod(blink_clock_, blink);
    }
    return std::nullopt;
}

void TimePicker::paint_period(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (is_24_hour_) {
        return;
    }
    const double e = enabled_.amount();
    const calendar::PickerStrings& s = locale_->strings;
    for (int i = 0; i < 2; ++i) {
        const render::Rect r = period_bounds(i == 1);
        const double checked = std::clamp(periods_[i].checked.value(), 0.0, 1.0);
        const double pressed = std::clamp(periods_[i].corner.value(), 0.0, 1.0);
        const double shape = std::max(checked, pressed);
        const double full = std::min(r.width, r.height) / 2.0;
        const double radius = full + (t.period_checked_corner - full) * shape;
        const color::Argb container = dim(color::mix(scheme[k.period_container], scheme[k.period_selected_container], checked), e);
        const color::Argb content = dim(color::mix(scheme[k.period_content], scheme[k.period_selected_content], checked), e);
        const render::Path path = render::rounded_rect_path(r, render::uniform_radii(std::max(0.0, radius)));
        canvas.fill_path(path, color::state_layer(container, content, periods_[i].state_layer.value()));
        text::TextStyle style = style_of(t.period_type);
        if (checked >= 0.5) {
            style.weight = t.period_selected_weight;
        }
        const text::TextLine line = text::layout_line(fonts_, i == 0 ? s.am : s.pm, style);
        text::draw_line(canvas, fonts_, line, {r.x + (r.width - line.width) / 2.0, r.y + (r.height - line.box_height()) / 2.0}, content);
        if (period_focus_.focused() && (i == 1) == is_pm()) {
            period_focus_.paint(canvas, scheme, r, std::max(0.0, radius));
        }
    }
}

void TimePicker::paint_display(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double e = enabled_.amount();
    const text::TextStyle style = style_of(t.selector_type);
    const double corner = tokens::corner_radius(t.selector_corner, t.selector_width, t.selector_height);
    const std::string texts[2] = {two_digits(display_hour()), two_digits(minute_)};
    for (int i = 0; i < 2; ++i) {
        const render::Rect r = field_bounds(i);
        const double sel = std::clamp(selectors_[i].checked.value(), 0.0, 1.0);
        const color::Argb container = dim(color::mix(scheme[k.time_container], scheme[k.time_selected_container], sel), e);
        const color::Argb content = dim(color::mix(scheme[k.time_content], scheme[k.time_selected_content], sel), e);
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(corner)),
                         color::state_layer(container, content, selector_focus_[i].state_layer_opacity(selectors_[i].state_layer.value())));
        const text::TextLine line = text::layout_line(fonts_, texts[i], style);
        text::draw_line(canvas, fonts_, line, {r.x + (r.width - line.width) / 2.0, r.y + (r.height - line.box_height()) / 2.0}, content);
        selector_focus_[i].paint(canvas, scheme, r, corner);
    }
    const render::Rect h = field_bounds(0);
    const text::TextLine colon = text::layout_line(fonts_, ":", style);
    text::draw_line(canvas, fonts_, colon,
                    {h.x + h.width + (t.separator_width - colon.width) / 2.0, h.y + (h.height - colon.box_height()) / 2.0 + t.separator_offset},
                    dim(scheme[k.separator], e));
    paint_period(canvas, scheme);
}

void TimePicker::paint_labels(render::Canvas& canvas, TimeSelection selection, double alpha, color::Argb color) const {
    if (alpha <= 0.0) {
        return;
    }
    const std::vector<DialLabel> labels = labels_for(selection);
    const text::TextStyle style = style_of(t.dial_label_type);
    for (const DialLabel& l : labels) {
        const text::TextLine line = text::layout_line(fonts_, calendar::format_number(l.value), style);
        text::draw_line(canvas, fonts_, line, {l.center.x - line.width / 2.0, l.center.y - line.box_height() / 2.0}, color::scale_alpha(color, alpha));
    }
}

void TimePicker::paint_dial(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double e = enabled_.amount();
    const render::Point c = dial_center();
    canvas.fill_path(render::circle_path(c, t.dial_size / 2.0), dim(scheme[k.dial], e));
    const bool inner = is_24_hour_ && selection_ == TimeSelection::hour && hour_ >= 12;
    const double handle_r = t.handle_size / 2.0;
    const double length = std::max(handle_r, t.dial_size * (inner ? t.inner_ratio : t.outer_ratio));
    const render::Point handle = on_circle(c, length, angle_.value());
    const double n = std::clamp(numbers_.value(), 0.0, 1.0);
    const color::Argb label = dim(scheme[k.dial_content], e);
    const color::Argb selected = dim(scheme[k.dial_selected_content], e);
    const color::Argb selector = dim(scheme[k.selector], e);
    paint_labels(canvas, previous_selection_, previous_selection_ == selection_ ? 0.0 : 1.0 - n, label);
    paint_labels(canvas, selection_, previous_selection_ == selection_ ? 1.0 : n, label);
    canvas.fill_path(render::circle_path(handle, handle_r), selector);
    canvas.push_clip(render::circle_path(handle, handle_r));
    paint_labels(canvas, previous_selection_, previous_selection_ == selection_ ? 0.0 : 1.0 - n, selected);
    paint_labels(canvas, selection_, previous_selection_ == selection_ ? 1.0 : n, selected);
    canvas.pop_clip();
    const render::Point edge = on_circle(c, length - handle_r, angle_.value());
    if (length - handle_r > 0.0) {
        const std::array<render::Point, 2> line{c, edge};
        canvas.fill_path(render::stroke_path(line, t.track_width), selector);
    }
    canvas.fill_path(render::circle_path(c, t.center_size / 2.0), selector);
    if (dial_focus_.focused()) {
        const std::vector<DialLabel> labels = dial_labels();
        const DialLabel& l = labels[std::min(dial_focus_index_, labels.size() - 1)];
        dial_focus_.paint_circle(canvas, scheme, l.center, t.label_size / 2.0);
    }
}

void TimePicker::paint_inputs(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double e = enabled_.amount();
    const text::TextStyle style = style_of(t.input_type);
    const double corner = tokens::corner_radius(t.input_field_corner, t.input_field_width, t.input_field_height);
    const bool valid[2] = {hour_valid(), minute_valid()};
    const std::string supporting[2] = {valid[0] ? locale_->strings.hour : hour_error(), valid[1] ? locale_->strings.minute : minute_error()};
    for (int i = 0; i < 2; ++i) {
        const render::Rect r = field_bounds(i);
        const bool active = fields_[i].focused;
        color::Argb container = active ? scheme[k.time_selected_container] : scheme[k.time_container];
        color::Argb content = active ? scheme[k.time_selected_content] : scheme[k.time_content];
        if (!valid[i]) {
            container = scheme[k.error_container];
            content = active ? scheme[k.error] : scheme[k.on_error_container];
        }
        const render::Path shape = render::rounded_rect_path(r, render::uniform_radii(corner));
        canvas.fill_path(shape, dim(container, e));
        if (active) {
            const color::Argb outline = valid[i] ? scheme[k.input_outline] : scheme[k.error];
            canvas.fill_path(render::ring_path(r, corner, t.input_focus_outline), dim(outline, e), render::FillRule::even_odd);
        }
        const text::TextLine line = text::layout_line(fonts_, fields_[i].text, style);
        const double x = r.x + (r.width - line.width) / 2.0;
        text::draw_line(canvas, fonts_, line, {x, r.y + (r.height - line.box_height()) / 2.0}, dim(content, e));
        const double blink = tokens::outlined_text_field.caret_blink_seconds;
        if (active && std::fmod(blink_clock_, 2.0 * blink) < blink) {
            canvas.fill_rect({x + line.width, r.y + r.height * t.caret_inset, t.caret_width, r.height * (1.0 - 2.0 * t.caret_inset)},
                             scheme[k.selector]);
        }
        const text::TextBlock block = text::layout_paragraph(fonts_, supporting[i], style_of(t.input_supporting_type), r.width);
        double y = r.y + r.height + t.input_supporting_top;
        for (const text::TextLine& l : block.lines) {
            text::draw_line(canvas, fonts_, l, {r.x, y}, dim(valid[i] ? scheme[k.supporting] : scheme[k.error], e));
            y += l.box_height();
        }
    }
    const render::Rect h = field_bounds(0);
    const text::TextLine colon = text::layout_line(fonts_, ":", style);
    text::draw_line(canvas, fonts_, colon,
                    {h.x + h.width + (t.separator_width - colon.width) / 2.0, h.y + (h.height - colon.box_height()) / 2.0 + t.separator_offset},
                    dim(scheme[k.separator], e));
    paint_period(canvas, scheme);
}

void TimePicker::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double a = std::clamp(mode_alpha_.value(), 0.0, 1.0);
    const color::ColorScheme faded = a < 1.0 ? color::faded(scheme, a) : scheme;
    if (mode_ == TimePickerMode::input) {
        paint_inputs(canvas, faded);
        return;
    }
    paint_display(canvas, faded);
    paint_dial(canvas, faded);
}

}
