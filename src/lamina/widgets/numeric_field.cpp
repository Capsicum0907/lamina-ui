#include "lamina/style/style.hpp"
#include "lamina/widgets/numeric_field.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/font.hpp"

namespace lamina::widgets {

namespace {

const tokens::NumericFieldTokens& t() {
    return current_style().numeric_field;
}

std::optional<double> parse_double(std::string_view s) {
    double v = 0.0;
#if defined(__cpp_lib_to_chars)
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
        return std::nullopt;
    }
#else
    constexpr std::string_view number_chars = "0123456789.eE+-";
    if (s.empty() || s.front() == '+' || s.find_first_not_of(number_chars) != std::string_view::npos) {
        return std::nullopt;
    }
    const std::string copy(s);
    char* end = nullptr;
    v = std::strtod(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size()) {
        return std::nullopt;
    }
#endif
    return v;
}

text::TextStyle style_of(tokens::TypeRole role) {
    return current_style().type.text(role);
}

std::optional<char32_t> numeric_character(char32_t c) {
    if (const std::optional<char32_t> d = ascii_digit(c)) {
        return d;
    }
    switch (c) {
    case U'.':
    case U'-':
    case U'+':
        return c;
    case U'．':
        return U'.';
    case U'－':
    case U'−':
        return U'-';
    case U'＋':
        return U'+';
    default:
        return std::nullopt;
    }
}

}

NumericField::NumericField(NumericFieldOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts_, options_.label, style_of(t().label_type))),
      unit_(text::layout_line(fonts_, options_.unit, style_of(current_style().text_field.input_type))),
      editor_(TextFieldOptions{.clipboard = options_.clipboard,
                               .enabled = options_.enabled,
                               .accessible_name = options_.accessible_name.empty() ? options_.label : options_.accessible_name,
                               .filter = numeric_character,
                               .min_width = 0.0,
                               .outlined = false,
                               .padding_start = 0.0,
                               .padding_end = 0.0,
                               .density = t().editor_density},
              fonts_),
      value_(0.0),
      enabled_(options_.enabled),
      hover_(0.0, t().color_spring),
      focus_(0.0, t().color_spring),
      error_amount_(0.0, t().color_spring) {
    value_ = clamp(rounded(options_.value));
    show_value();
    place_editor();
}

double NumericField::label_space() const {
    if (options_.label.empty()) {
        return 0.0;
    }
    return options_.label_width.value_or(label_.width);
}

double NumericField::unit_space() const {
    return options_.unit.empty() ? 0.0 : unit_.width + t().unit_gap;
}

double NumericField::width() const {
    const double field = options_.width.value_or(t().width);
    if (options_.label_position == NumericLabelPosition::start && !options_.label.empty()) {
        return label_space() + t().label_gap + field;
    }
    return field;
}

double NumericField::height() const {
    return t().height.at(options_.density);
}

render::Rect NumericField::field_rect() const {
    const double field = options_.width.value_or(t().width);
    return {origin_.x + width() - field, origin_.y, field, height()};
}

render::Rect NumericField::label_rect() const {
    if (options_.label.empty()) {
        return {origin_.x, origin_.y, 0.0, 0.0};
    }
    if (options_.label_position == NumericLabelPosition::start) {
        return {origin_.x, origin_.y, label_space(), height()};
    }
    const render::Rect f = field_rect();
    return {f.x + t().padding, f.y, label_space(), f.height};
}

void NumericField::place_editor() {
    const render::Rect f = field_rect();
    double left = f.x + t().padding;
    if (options_.label_position == NumericLabelPosition::inside && !options_.label.empty()) {
        left += label_space() + t().inline_label_gap;
    }
    const double right = f.x + f.width - t().padding - unit_space();
    editor_.set_width(std::max(0.0, right - left));
    editor_.set_origin({left, f.y + (f.height - editor_.height()) / 2.0});
}

void NumericField::arrange(const render::Rect& r) {
    set_origin({r.x, r.y + (r.height - height()) / 2.0});
}

void NumericField::set_origin(render::Point origin) {
    origin_ = origin;
    place_editor();
}

void NumericField::set_density(tokens::Density density) {
    options_.density = density;
    place_editor();
}

double NumericField::clamp(double v) const {
    return std::clamp(v, options_.min, options_.max);
}

double NumericField::rounded(double v) const {
    const double k = std::pow(10.0, options_.decimals);
    const double r = std::round(v * k) / k;
    return r == 0.0 ? 0.0 : r;
}

std::string NumericField::format(double v) const {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", std::max(0, options_.decimals), v);
    return buf;
}

std::string NumericField::formatted() const {
    return format(value_);
}

std::optional<double> NumericField::parse(const std::string& s) const {
    std::size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::nullopt;
    }
    std::size_t e = s.find_last_not_of(' ') + 1;
    if (s[b] == '+') {
        ++b;
    }
    const std::optional<double> v = parse_double(std::string_view(s).substr(b, e - b));
    if (!v || !std::isfinite(*v)) {
        return std::nullopt;
    }
    return v;
}

void NumericField::show_value() {
    editor_.set_text(formatted());
    error_ = false;
    editor_.set_error(false);
    error_amount_.animate_to(0.0);
    if (editor_.focused()) {
        const std::size_t n = editor_.text().size();
        editor_.select(0, n);
    }
}

void NumericField::set_value(double v) {
    value_ = clamp(rounded(v));
    show_value();
}

bool NumericField::change_to(double v) {
    const double next = clamp(rounded(v));
    const bool changed = next != value_;
    value_ = next;
    show_value();
    return changed;
}

bool NumericField::commit() {
    const std::optional<double> v = parse(editor_.text());
    if (!v) {
        error_ = true;
        editor_.set_error(true);
        error_amount_.animate_to(1.0);
        return false;
    }
    change_to(*v);
    return true;
}

void NumericField::revert() {
    show_value();
}

std::optional<input::Cursor> NumericField::cursor() const {
    if (!enabled()) {
        return std::nullopt;
    }
    if (scrubbing_) {
        return input::Cursor::resize_horizontal;
    }
    if (editing() && hovered_) {
        return input::Cursor::text;
    }
    if (hovered_ || label_hovered_) {
        return input::Cursor::resize_horizontal;
    }
    return std::nullopt;
}

void NumericField::set_focused(bool focused) {
    focused = focused && enabled();
    if (focused == editor_.focused()) {
        return;
    }
    if (!focused && !commit()) {
        revert();
    }
    editor_.set_focused(focused);
    focus_.animate_to(focused ? 1.0 : 0.0);
    if (focused) {
        editor_.select(0, editor_.text().size());
    }
}

void NumericField::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    editor_.set_enabled(enabled);
    if (!enabled) {
        set_focused(false);
        cancel_interaction();
    }
}

AccessibleNode NumericField::accessible() const {
    AccessibleState state{.enabled = enabled(), .focusable = enabled(), .focused = focused(), .invalid = error_};
    state.range = AccessibleRange{value_, options_.min, options_.max};
    const std::string shown = editing() ? editor_.text() : formatted();
    return {AccessibleRole::spin_button, options_.accessible_name.empty() ? options_.label : options_.accessible_name, bounds(), state,
            options_.unit.empty() ? shown : shown + " " + options_.unit};
}

NumericFieldResponse NumericField::handle(const input::PointerEvent& e) {
    NumericFieldResponse r;
    if (!enabled()) {
        return r;
    }
    const bool leave = e.action == input::PointerAction::leave;
    const render::Rect f = field_rect();
    const bool over_field = !leave && contains(f, e.x_dp, e.y_dp);
    const bool over_label =
        !leave && options_.label_position == NumericLabelPosition::start && !options_.label.empty() && contains(label_rect(), e.x_dp, e.y_dp);
    hovered_ = over_field;
    label_hovered_ = over_label;
    hover_.animate_to(over_field || over_label ? 1.0 : 0.0);
    if (leave) {
        pressed_ = false;
        scrubbing_ = false;
        editor_.handle(e);
        return r;
    }
    if (pressed_) {
        const double dx = e.x_dp - press_x_;
        if (!scrubbing_ && std::abs(dx) > t().scrub_slop) {
            scrubbing_ = true;
        }
        if (scrubbing_ && e.action != input::PointerAction::up) {
            const double steps = std::trunc(dx / t().scrub_distance);
            r.changed = change_to(press_value_ + steps * options_.step);
        }
        if (e.action == input::PointerAction::up) {
            pressed_ = false;
            if (scrubbing_) {
                scrubbing_ = false;
                r.committed = true;
            } else if (over_field || over_label) {
                editor_.set_focused(true);
                focus_.animate_to(1.0);
                editor_.select(0, editor_.text().size());
            }
        }
        r.consumed = true;
        return r;
    }
    if (editing()) {
        if (e.action == input::PointerAction::down && !over_field) {
            const bool ok = commit();
            if (!ok) {
                revert();
            }
            r.committed = true;
            editor_.set_focused(false);
            focus_.animate_to(0.0);
            if (!over_label) {
                return r;
            }
        } else {
            editor_.handle(e);
            r.consumed = over_field;
            return r;
        }
    }
    if (e.action == input::PointerAction::down && (over_field || over_label)) {
        pressed_ = true;
        press_x_ = e.x_dp;
        press_value_ = value_;
        r.consumed = true;
    }
    return r;
}

NumericFieldResponse NumericField::handle(const input::KeyEvent& e) {
    NumericFieldResponse r;
    if (!enabled() || !editing()) {
        return r;
    }
    const bool up = e.key == input::Key::up || e.key == input::Key::page_up;
    const bool down = e.key == input::Key::down || e.key == input::Key::page_down;
    if (up || down) {
        const bool large = e.shift || e.key == input::Key::page_up || e.key == input::Key::page_down;
        const double base = parse(editor_.text()).value_or(value_);
        const double amount = large ? options_.large_step : options_.step;
        r.changed = change_to(base + (up ? amount : -amount));
        r.consumed = true;
        return r;
    }
    if (e.key == input::Key::enter && !e.repeat) {
        const double before = value_;
        commit();
        r.changed = before != value_;
        r.committed = true;
        r.consumed = true;
        if (!error_) {
            editor_.select(0, editor_.text().size());
        }
        return r;
    }
    if (e.key == input::Key::escape) {
        revert();
        r.consumed = true;
        return r;
    }
    if (e.key == input::Key::tab) {
        return r;
    }
    r.consumed = editor_.handle(e);
    return r;
}

void NumericField::handle(const input::TextEvent& e) {
    if (editing() && enabled()) {
        editor_.handle(e);
    }
}

void NumericField::cancel_interaction() {
    pressed_ = false;
    scrubbing_ = false;
    hovered_ = false;
    label_hovered_ = false;
    hover_.animate_to(0.0);
    editor_.cancel_interaction();
}

void NumericField::step(double dt) {
    editor_.step(dt);
    enabled_.step(dt);
    hover_.step(dt);
    focus_.step(dt);
    error_amount_.step(dt);
}

bool NumericField::animating() const {
    return editor_.animating() || !enabled_.settled() || !hover_.settled() || !focus_.settled() || !error_amount_.settled();
}

void NumericField::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect f = field_rect();
    const double radius = current_style().shape.radius(t().corner, f.width, f.height);
    const double focus = std::clamp(focus_.value(), 0.0, 1.0);
    const double err = std::clamp(error_amount_.value(), 0.0, 1.0);
    const double hover = std::clamp(hover_.value(), 0.0, 1.0);
    const color::Argb idle = color::mix(scheme[t().outline], scheme[t().hover_outline], hover);
    const color::Argb active = color::mix(color::mix(idle, scheme[t().focus_outline], focus), scheme[t().error], err);
    const color::Argb outline = enabled_.mix(resolve(scheme, t().disabled_outline), active);
    const double outline_width = motion::lerp(t().outline_width, t().focus_outline_width, std::max(focus, err));
    if (options_.label_position == NumericLabelPosition::inside) {
        const color::Argb fill = color::source_over(color::scale_alpha(scheme[t().hover_outline], t().state_layer * hover * enabled_.amount()),
                                                    scheme[t().inline_container]);
        canvas.fill_path(render::rounded_rect_path(f, render::uniform_radii(radius)), enabled_.mix(resolve(scheme, t().disabled_outline), fill));
        const double ring = motion::lerp(0.0, t().focus_outline_width, std::max(focus, err));
        if (ring > 0.0) {
            canvas.fill_path(render::ring_path(f, radius, ring), outline, render::FillRule::even_odd);
        }
    } else {
        canvas.fill_path(render::ring_path(f, radius, outline_width), outline, render::FillRule::even_odd);
    }
    const color::Argb label = enabled_.mix(resolve(scheme, t().disabled_content), color::mix(scheme[t().label], scheme[t().error], err));
    if (!options_.label.empty()) {
        const render::Rect l = label_rect();
        canvas.save();
        canvas.push_clip(options_.label_position == NumericLabelPosition::start ? render::Rect{l.x, l.y, l.width, l.height} : f);
        text::draw_line(canvas, fonts_, label_, {l.x, l.y + (l.height - label_.box_height()) / 2.0}, label);
        canvas.pop_clip();
        canvas.restore();
    }
    editor_.paint(canvas, scheme);
    if (!options_.unit.empty()) {
        const color::Argb unit = enabled_.mix(resolve(scheme, t().disabled_content), scheme[t().unit]);
        text::draw_line(canvas, fonts_, unit_, {f.x + f.width - t().padding - unit_.width, f.y + (f.height - unit_.box_height()) / 2.0}, unit);
    }
}

}
