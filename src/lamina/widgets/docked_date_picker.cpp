#include "lamina/style/style.hpp"
#include "lamina/widgets/docked_date_picker.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/date_picker.hpp"
#include "lamina/tokens/dialog.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::DatePickerTokens& t() {
    return current_style().date_picker;
}
const tokens::DatePickerColors& k() {
    return current_style().date_picker_colors;
}
const tokens::DatePickerMotion& m() {
    return current_style().date_picker_motion;
}
double closed_scale() {
    return current_style().dialog.closed_scale;
}

const calendar::CalendarLocale& locale_or_default(const calendar::CalendarLocale* l) {
    return l ? *l : calendar::english_us();
}

DatePickerOptions picker_options(const DockedDatePickerOptions& o) {
    return {.today = o.today,
            .selected = o.selected,
            .year_range = o.year_range,
            .selectable = o.selectable,
            .show_header = false,
            .show_mode_toggle = false,
            .locale = o.locale,
            .clipboard = o.clipboard,
            .enabled = o.enabled};
}

}

DockedDatePicker::DockedDatePicker(DockedDatePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)),
      fonts_(fonts),
      icon_font_(icon_font),
      locale_(&locale_or_default(options_.locale)),
      selected_(options_.selected),
      open_button_({{icon_font, t().docked_symbol},
                    options_.open_name.empty() ? locale_->strings.date_picker_title : options_.open_name,
                    tokens::IconButtonStyle::standard, tokens::IconButtonSize::small, tokens::ButtonShape::round,
                    tokens::IconButtonWidth::uniform, false, false, options_.enabled, color::Role::on_surface_variant}),
      picker_(picker_options(options_), fonts, icon_font),
      popup_(closed_scale(), m().popup_scale, m().popup_fade),
      enabled_(options_.enabled) {
    rebuild_field();
    place();
}

void DockedDatePicker::rebuild_field() {
    const std::string placeholder = calendar::input_placeholder(locale_->input);
    TextFieldOptions o{.label = locale_->strings.date_input_label,
                       .placeholder = placeholder,
                       .clipboard = options_.clipboard,
                       .enabled = enabled_,
                       .accessible_name = locale_->strings.date_input_label + locale_->strings.list_separator + placeholder,
                       .filter = ascii_digit,
                       .mask = locale_->input.pattern,
                       .min_width = t().container_width};
    const bool had_focus = field_ && field_->focused();
    field_ = std::make_unique<TextField>(std::move(o), fonts_);
    field_->set_width(t().container_width);
    sync_field();
    if (had_focus) {
        field_->set_focused(true);
    }
}

void DockedDatePicker::sync_field() {
    field_->set_text(selected_ ? calendar::input_digits(locale_->input, *selected_) : std::string());
    error_.clear();
    field_->set_supporting_text({});
    field_->set_error(false);
}

void DockedDatePicker::set_locale(const calendar::CalendarLocale& locale) {
    locale_ = &locale;
    open_button_.set_accessible_name(options_.open_name.empty() ? locale_->strings.date_picker_title : options_.open_name);
    picker_.set_locale(locale);
    rebuild_field();
    place();
}

double DockedDatePicker::width() const {
    return t().container_width;
}

double DockedDatePicker::height() const {
    return field_->height();
}

void DockedDatePicker::set_origin(render::Point origin) {
    origin_ = origin;
    place();
}

layout::Size DockedDatePicker::measure(const layout::Constraints&) const {
    return {width(), height()};
}

void DockedDatePicker::arrange(const render::Rect& r) {
    set_origin({r.x, r.y});
}

render::Rect DockedDatePicker::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

void DockedDatePicker::place() {
    field_->set_origin(origin_);
    const double field_h = field_->container_height();
    open_button_.set_origin({origin_.x + t().container_width - t().docked_icon_inset - open_button_.width() / 2.0,
                             origin_.y + (field_h - open_button_.height()) / 2.0});
    picker_.set_origin({origin_.x, origin_.y + height() + t().docked_gap});
}

render::Rect DockedDatePicker::popup_bounds() const {
    return picker_.bounds();
}

void DockedDatePicker::set_selected(std::optional<calendar::Date> date) {
    selected_ = date;
    picker_.set_selected(date);
    if (date) {
        picker_.set_displayed_month(calendar::year_month(*date), false);
    }
    sync_field();
}

void DockedDatePicker::open(input::InputSource source) {
    if (!enabled_) {
        return;
    }
    if (selected_) {
        picker_.set_displayed_month(calendar::year_month(*selected_), false);
    }
    popup_.open();
    if (source == input::InputSource::keyboard) {
        open_button_.set_focused(false);
        picker_.focus_grid();
    }
}

void DockedDatePicker::close() {
    if (!popup_.is_open()) {
        return;
    }
    const bool had_focus = picker_.focused();
    popup_.close();
    picker_.clear_focus();
    picker_.set_year_picker_open(false);
    picker_.cancel_interaction();
    if (had_focus) {
        open_button_.set_focused(true);
    }
}

void DockedDatePicker::add_focus_stops(FocusManager& focus) {
    field_stop_ = std::make_unique<FocusStop>(FocusStop::Hooks{[this] { return field_->focused(); }, [this](bool f) { field_->set_focused(f); },
                                                               [this] { return field_->can_focus(); }, [this] { return field_->bounds(); }});
    focus.add(*field_stop_);
    focus.add(open_button_);
    picker_.add_focus_stops(focus);
}

bool DockedDatePicker::focused() const {
    return field_->focused() || open_button_.focused() || picker_.focused();
}

void DockedDatePicker::set_enabled(bool enabled) {
    enabled_ = enabled;
    field_->set_enabled(enabled);
    open_button_.set_enabled(enabled);
    picker_.set_enabled(enabled);
    if (!enabled) {
        close();
    }
}

AccessibleNode DockedDatePicker::accessible() const {
    AccessibleNode node{AccessibleRole::group, locale_->strings.date_input_label, bounds(), {.enabled = enabled_}};
    node.children.push_back(field_->accessible());
    node.children.push_back(open_button_.accessible());
    if (popup_.is_open()) {
        node.children.push_back(picker_.accessible());
    }
    return node;
}

void DockedDatePicker::on_field_changed(DatePickerResponse& response) {
    const std::string digits = field_->text();
    std::optional<calendar::Date> parsed;
    std::string error;
    if (digits.size() == calendar::input_pattern_digits(locale_->input)) {
        error = validate_date_input(*locale_, digits, options_.year_range, options_.selectable, &parsed);
    }
    error_ = error;
    field_->set_supporting_text(error);
    field_->set_error(!error.empty());
    if (parsed != selected_) {
        selected_ = parsed;
        picker_.set_selected(parsed);
        if (parsed) {
            picker_.set_displayed_month(calendar::year_month(*parsed), false);
        }
        response.changed = true;
    }
}

DatePickerResponse DockedDatePicker::handle(const input::PointerEvent& e) {
    DatePickerResponse response;
    if (!enabled_) {
        return response;
    }
    if (popup_.is_open()) {
        if (popup_.dismisses(e, popup_bounds()) && !contains(touch_target(open_button_.bounds()), e.x_dp, e.y_dp)) {
            close();
        } else if (contains(popup_bounds(), e.x_dp, e.y_dp) || e.action != input::PointerAction::down) {
            const std::optional<calendar::Date> before = picker_.selected();
            const DatePickerResponse r = picker_.handle(e);
            if (r.changed && picker_.selected() != before) {
                selected_ = picker_.selected();
                sync_field();
                response.changed = true;
                close();
            }
            if (contains(popup_bounds(), e.x_dp, e.y_dp)) {
                response.consumed = true;
                return response;
            }
        }
    }
    if (open_button_.handle(e)) {
        if (popup_.is_open()) {
            close();
        } else {
            open();
        }
        response.consumed = true;
        return response;
    }
    const bool over_button = e.action != input::PointerAction::leave && contains(touch_target(open_button_.bounds()), e.x_dp, e.y_dp);
    if (over_button) {
        if (e.action == input::PointerAction::down) {
            field_->set_focused(false);
        }
        field_->handle(input::PointerEvent{input::PointerAction::leave, e.x_dp, e.y_dp});
        response.consumed = true;
        return response;
    }
    const std::string before = field_->text();
    if (field_->handle(e)) {
        response.consumed = true;
    }
    if (field_->text() != before) {
        on_field_changed(response);
    }
    return response;
}

DatePickerResponse DockedDatePicker::handle(const input::KeyEvent& e) {
    DatePickerResponse response;
    if (!enabled_) {
        return response;
    }
    if (popup_.is_open() && e.key == input::Key::escape) {
        close();
        response.consumed = true;
        return response;
    }
    if (picker_.focused()) {
        const std::optional<calendar::Date> before = picker_.selected();
        const DatePickerResponse r = picker_.handle(e);
        if (r.changed && picker_.selected() != before) {
            selected_ = picker_.selected();
            sync_field();
            response.changed = true;
            close();
        }
        response.consumed = r.consumed || response.changed;
        return response;
    }
    if (open_button_.focused() && open_button_.handle(e)) {
        if (popup_.is_open()) {
            close();
        } else {
            open(input::InputSource::keyboard);
        }
        response.consumed = true;
        return response;
    }
    if (field_->focused()) {
        if (e.key == input::Key::down && e.alt) {
            open(input::InputSource::keyboard);
            response.consumed = true;
            return response;
        }
        const std::string before = field_->text();
        field_->handle(e);
        response.consumed = e.key != input::Key::escape && e.key != input::Key::tab;
        if (field_->text() != before) {
            on_field_changed(response);
        }
    }
    return response;
}

void DockedDatePicker::handle(const input::TextEvent& e) {
    if (!field_->focused()) {
        return;
    }
    const std::string before = field_->text();
    field_->handle(e);
    if (field_->text() != before) {
        DatePickerResponse ignored;
        on_field_changed(ignored);
    }
}

bool DockedDatePicker::handle(const input::WheelEvent& e) {
    return popup_.is_open() && picker_.handle(e);
}

void DockedDatePicker::cancel_interaction() {
    field_->cancel_interaction();
    open_button_.cancel_interaction();
    picker_.cancel_interaction();
}

void DockedDatePicker::step(double dt) {
    field_->step(dt);
    open_button_.step(dt);
    popup_.step(dt);
    if (popup_.visible()) {
        picker_.step(dt);
    }
    place();
}

bool DockedDatePicker::animating() const {
    return field_->animating() || open_button_.animating() || !popup_.settled() || (popup_.visible() && picker_.animating());
}

std::optional<render::Rect> DockedDatePicker::caret_area() const {
    return field_->focused() ? std::optional<render::Rect>(field_->caret_area()) : std::nullopt;
}

std::optional<double> DockedDatePicker::next_wake_seconds() const {
    std::optional<double> wake;
    if (field_->focused()) {
        wake = field_->next_blink_seconds();
    }
    if (popup_.visible()) {
        if (const auto w = picker_.next_wake_seconds()) {
            wake = wake ? std::min(*wake, *w) : *w;
        }
    }
    return wake;
}

void DockedDatePicker::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    field_->paint(canvas, scheme);
    open_button_.paint(canvas, scheme);
}

void DockedDatePicker::paint_popup(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!popup_.visible()) {
        return;
    }
    const render::Rect b = popup_bounds();
    const double a = popup_.opacity();
    const color::ColorScheme faded = color::faded(scheme, a);
    canvas.save();
    popup_.scale_about(canvas, {b.x + b.width / 2.0, b.y});
    const double radius = current_style().shape.radius(t().docked_corner, b.width, b.height);
    paint_surface(canvas, b, radius, faded[k().container], t().docked_elevation, faded[color::Role::shadow]);
    const render::Path shape = render::rounded_rect_path(b, render::uniform_radii(radius));
    canvas.push_clip(shape);
    picker_.paint(canvas, faded);
    canvas.pop_clip();
    canvas.restore();
}

}
