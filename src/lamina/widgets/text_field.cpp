#include "lamina/widgets/text_field.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

const auto& t = tokens::outlined_text_field;
using tokens::motion_scheme;

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

void append_segments(render::Path& out, const render::Path& contour) {
    const std::vector<render::PathVerb>& verbs = contour.verbs();
    const std::vector<render::Point>& points = contour.points();
    std::size_t i = 1;
    for (std::size_t v = 1; v < verbs.size(); ++v) {
        switch (verbs[v]) {
        case render::PathVerb::line:
            out.line_to(points[i++]);
            break;
        case render::PathVerb::cubic:
            out.cubic_to(points[i], points[i + 1], points[i + 2]);
            i += 3;
            break;
        case render::PathVerb::move:
        case render::PathVerb::close:
            break;
        }
    }
}

render::Path notched_ring_path(const render::Rect& r, double radius, double width, double gap_left, double gap_right) {
    const double straight = std::max(radius, width);
    const double a = std::max(gap_left, r.x + straight);
    const double b = std::min(gap_right, r.x + r.width - straight);
    if (a >= b) {
        return render::ring_path(r, radius, width);
    }
    const render::Rect inner{r.x + width, r.y + width, r.width - 2 * width, r.height - 2 * width};
    const render::Affine mirror{-1.0, 0.0, 0.0, 1.0, 2 * inner.x + inner.width, 0.0};
    render::Path p;
    p.move_to({b, r.y});
    append_segments(p, render::rounded_rect_path(r, render::uniform_radii(radius)));
    p.line_to({a, r.y});
    p.line_to({a, inner.y});
    append_segments(p, render::rounded_rect_path(inner, render::uniform_radii(std::max(0.0, radius - width))).transformed(mirror));
    p.line_to({b, inner.y});
    p.close();
    return p;
}

bool is_slot(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::size_t slot_count(const std::string& mask) {
    return static_cast<std::size_t>(std::count_if(mask.begin(), mask.end(), is_slot));
}

enum class Shortcut { none, select_all, copy, cut, paste, undo, redo };

Shortcut shortcut(const input::KeyEvent& e) {
    if (!e.ctrl) {
        return Shortcut::none;
    }
    switch (e.key) {
    case input::Key::a: return Shortcut::select_all;
    case input::Key::c: return Shortcut::copy;
    case input::Key::x: return Shortcut::cut;
    case input::Key::v: return Shortcut::paste;
    case input::Key::z: return e.shift ? Shortcut::redo : Shortcut::undo;
    case input::Key::y: return e.shift ? Shortcut::none : Shortcut::redo;
    default: return Shortcut::none;
    }
}

}

std::optional<char32_t> ascii_digit(char32_t c) {
    if (c >= U'0' && c <= U'9') {
        return c;
    }
    if (c >= U'０' && c <= U'９') {
        return static_cast<char32_t>(U'0' + (c - U'０'));
    }
    return std::nullopt;
}

TextField::TextField(TextFieldOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      width_(options_.min_width),
      buffer_({.multiline = options_.multiline,
               .keeps_tabs = options_.accepts_tab,
               .history = {.depth = t.undo_depth, .pause_seconds = t.undo_pause_seconds}}),
      label_large_(text::layout_line(fonts, options_.label, style_of(t.label_type))),
      label_small_(text::layout_line(fonts, options_.label, style_of(t.minimized_label_type))),
      placeholder_(text::layout_line(fonts, options_.placeholder, style_of(t.input_type))),
      label_progress_(0.0, motion_scheme.fast_spatial()),
      focus_(0.0, motion_scheme.fast_effects()),
      hover_(0.0, motion_scheme.fast_effects()),
      placeholder_opacity_(0.0, motion_scheme.fast_effects()),
      enabled_(options_.enabled),
      supporting_(text::layout_line(fonts, options_.supporting_text, style_of(t.supporting_type))),
      error_amount_(0.0, motion_scheme.default_effects()) {
    if (masked()) {
        options_.multiline = false;
    }
    relayout();
    phase_ = input_phase();
    placeholder_opacity_.snap_to(phase_ == InputPhase::unfocused_not_empty || (phase_ == InputPhase::unfocused_empty && !options_.label.empty())
                                     ? 0.0
                                     : 1.0);
}

TextField::InputPhase TextField::input_phase() const {
    if (focused_) {
        return InputPhase::focused;
    }
    return buffer_.text().empty() && !buffer_.composing() ? InputPhase::unfocused_empty : InputPhase::unfocused_not_empty;
}

void TextField::update_phase() {
    const InputPhase next = input_phase();
    if (next == phase_) {
        return;
    }
    const bool slow = (phase_ == InputPhase::unfocused_empty && next == InputPhase::focused) ||
                      (phase_ == InputPhase::unfocused_not_empty && next == InputPhase::unfocused_empty);
    placeholder_opacity_.set_spec(slow ? motion_scheme.slow_effects() : motion_scheme.fast_effects());
    phase_ = next;
    switch (next) {
    case InputPhase::focused:
        placeholder_opacity_.animate_to(1.0);
        break;
    case InputPhase::unfocused_empty:
        placeholder_opacity_.animate_to(options_.label.empty() ? 1.0 : 0.0);
        break;
    case InputPhase::unfocused_not_empty:
        placeholder_opacity_.animate_to(0.0);
        break;
    }
}

bool TextField::placeholder_shown() const {
    return !options_.placeholder.empty() && buffer_.text().empty() && !buffer_.composing() && placeholder_opacity_.value() > 0.0;
}

void TextField::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        set_focused(false);
        cancel_interaction();
    }
}

double TextField::container_height() const {
    return options_.multiline ? std::max(t.height, content_height() + 2 * t.vertical_padding) : t.height;
}

double TextField::height() const {
    const double extra = options_.supporting_text.empty() ? 0.0 : t.supporting_top_padding + supporting_.box_height();
    return container_height() + extra;
}

void TextField::set_supporting_text(std::string text) {
    options_.supporting_text = std::move(text);
    supporting_ = text::layout_line(fonts_, options_.supporting_text, style_of(t.supporting_type));
}

void TextField::set_error(bool error) {
    error_ = error;
    error_amount_.animate_to(error ? 1.0 : 0.0);
}

std::size_t TextField::length_limit() const {
    return masked() ? slot_count(options_.mask) : options_.max_length;
}

std::u32string TextField::accepted(std::u32string_view in) const {
    std::u32string out;
    for (char32_t c : in) {
        if (options_.filter) {
            if (const std::optional<char32_t> mapped = options_.filter(c)) {
                out.push_back(*mapped);
            }
        } else {
            out.push_back(c);
        }
    }
    const std::size_t limit = length_limit();
    if (limit > 0) {
        const std::size_t kept = buffer_.text().size() - (buffer_.selection_end() - buffer_.selection_start());
        const std::size_t room = kept >= limit ? 0 : limit - kept;
        if (out.size() > room) {
            out.resize(room);
        }
    }
    return out;
}

std::size_t TextField::to_display(std::size_t raw) const {
    if (!masked()) {
        return raw;
    }
    const std::size_t length = buffer_.shown().size();
    std::size_t slots = 0;
    std::size_t display = 0;
    for (char c : options_.mask) {
        if (is_slot(c)) {
            if (slots >= length || slots >= raw) {
                break;
            }
            ++slots;
        }
        ++display;
    }
    return display;
}

std::size_t TextField::to_raw(std::size_t display) const {
    if (!masked()) {
        return display;
    }
    std::size_t raw = 0;
    for (std::size_t i = 0; i < display && i < options_.mask.size(); ++i) {
        raw += is_slot(options_.mask[i]) ? 1 : 0;
    }
    return std::min(raw, buffer_.shown().size());
}

std::string TextField::display_text() const {
    const std::u32string_view shown = buffer_.shown();
    if (!masked()) {
        return text::encode_utf8(shown);
    }
    std::u32string out;
    std::size_t used = 0;
    for (char c : options_.mask) {
        if (!is_slot(c)) {
            if (used == 0) {
                break;
            }
            out.push_back(static_cast<char32_t>(c));
            continue;
        }
        if (used >= shown.size()) {
            break;
        }
        out.push_back(shown[used++]);
    }
    return text::encode_utf8(out);
}

void TextField::set_width(double width) {
    width_ = std::max(width, options_.min_width);
    relayout();
}

render::Rect TextField::bounds() const {
    return {origin_.x, origin_.y, width(), container_height()};
}

double TextField::text_left() const {
    return origin_.x + t.padding;
}

double TextField::inner_width() const {
    return width_ - 2 * t.padding;
}

double TextField::content_height() const {
    return line_tops_.empty() ? 0.0 : line_tops_.back();
}

double TextField::input_top() const {
    return origin_.y + (container_height() - content_height()) / 2.0;
}

std::string TextField::text() const {
    return text::encode_utf8(buffer_.text());
}

void TextField::set_text(std::string_view utf8) {
    buffer_.set_text(text::decode_utf8(utf8));
    relayout();
}

void TextField::select(std::size_t anchor, std::size_t cursor) {
    buffer_.select(anchor, cursor);
    relayout();
}

std::string TextField::selected_text() const {
    return text::encode_utf8(buffer_.selected());
}

void TextField::set_focused(bool focused) {
    focused = focused && enabled();
    if (focused_ == focused) {
        return;
    }
    focused_ = focused;
    dragging_ = false;
    buffer_.close_step();
    if (!focused_) {
        buffer_.clear_composition();
        relayout();
    }
    reset_blink();
    focus_.animate_to(focused_ ? 1.0 : 0.0);
    label_progress_.animate_to(focused_ || !buffer_.text().empty() ? 1.0 : 0.0);
    update_phase();
}

void TextField::relayout() {
    const text::TextStyle style = style_of(t.input_type);
    const std::u32string_view shown = buffer_.shown();
    const std::string utf8 = display_text();
    if (options_.multiline) {
        lines_ = text::layout_paragraph(fonts_, utf8, style, inner_width()).lines;
    } else {
        lines_.assign(1, text::layout_line(fonts_, utf8, style));
    }
    std::vector<text::LineSpan> spans;
    spans.reserve(lines_.size());
    line_tops_.assign(1, 0.0);
    for (const text::TextLine& line : lines_) {
        spans.push_back(masked() ? text::LineSpan{0, shown.size()} : text::LineSpan{line.first, line.last});
        line_tops_.push_back(line_tops_.back() + line.box_height());
    }
    line_map_ = text::LineMap(shown, std::move(spans));
    update_scroll();
    label_progress_.animate_to(focused_ || !buffer_.text().empty() || buffer_.composing() ? 1.0 : 0.0);
    update_phase();
}

void TextField::update_scroll() {
    if (options_.multiline) {
        scroll_x_ = 0.0;
        return;
    }
    const std::size_t caret = buffer_.caret();
    const double x = x_in_line(line_map_.line_of(caret, false), caret);
    const double visible = inner_width();
    if (x - scroll_x_ > visible) {
        scroll_x_ = x - visible;
    }
    if (x < scroll_x_) {
        scroll_x_ = x;
    }
    scroll_x_ = std::clamp(scroll_x_, 0.0, std::max(0.0, std::max(lines_.front().width, x) - visible));
}

void TextField::reset_blink() {
    blink_clock_ = 0.0;
}

double TextField::x_in_line(std::size_t line, std::size_t shown) const {
    const text::TextLine& l = lines_[line];
    if (masked()) {
        return text::caret_x(l, to_display(shown));
    }
    if (shown <= l.last) {
        return text::caret_x(l, shown);
    }
    const std::size_t end = std::min(shown, line_map_.limit(line));
    const std::u32string_view run = buffer_.shown().substr(l.first, end - l.first);
    return text::layout_line(fonts_, text::encode_utf8(run), l.style).width;
}

std::size_t TextField::Geometry::index_at(std::size_t line, double x) const {
    return field_.to_raw(text::index_at(field_.lines_[line], x));
}

TextField::Hit TextField::hit(double x, double y) const {
    const double local_y = y - input_top();
    std::size_t k = 0;
    while (k + 1 < lines_.size() && local_y >= line_tops_[k + 1]) {
        ++k;
    }
    const std::size_t index = to_raw(text::index_at(lines_[k], x - text_left() + scroll_x_));
    return {index, line_map_.wraps_at(k, index)};
}

bool TextField::handles_history_key(const input::KeyEvent& e) const {
    if (!focused_) {
        return false;
    }
    switch (shortcut(e)) {
    case Shortcut::undo: return buffer_.composing() || buffer_.can_undo();
    case Shortcut::redo: return buffer_.composing() || buffer_.can_redo();
    default: return false;
    }
}

void TextField::copy_selection() const {
    if (options_.clipboard && buffer_.has_selection()) {
        options_.clipboard->set_text(selected_text());
    }
}

bool TextField::handle(const input::PointerEvent& e) {
    if (!enabled()) {
        return false;
    }
    press_.update(e, bounds());
    hover_.animate_to(press_.hovered ? 1.0 : 0.0);
    if (e.action == input::PointerAction::up) {
        dragging_ = false;
        return false;
    }
    if (e.action == input::PointerAction::move) {
        if (dragging_ && !buffer_.composing()) {
            const Hit h = hit(e.x_dp, e.y_dp);
            buffer_.move_to(h.index, true, h.upstream);
            relayout();
            reset_blink();
        }
        return false;
    }
    if (e.action != input::PointerAction::down) {
        return false;
    }
    if (!press_.hovered) {
        set_focused(false);
        return false;
    }
    set_focused(true);
    if (!buffer_.composing()) {
        const Hit h = hit(e.x_dp, e.y_dp);
        if (e.clicks == 2) {
            buffer_.select_word_at(h.index);
        } else if (e.clicks >= 3) {
            buffer_.select_all();
        } else {
            buffer_.move_to(h.index, false, h.upstream);
        }
        dragging_ = e.clicks <= 1;
        buffer_.reset_goal();
        relayout();
    }
    reset_blink();
    return true;
}

bool TextField::handle(const input::KeyEvent& e) {
    if (!focused_ || buffer_.composing()) {
        return false;
    }
    bool submit = false;
    switch (e.key) {
    case input::Key::backspace:
        buffer_.backspace(e.ctrl);
        break;
    case input::Key::delete_forward:
        buffer_.delete_forward(e.ctrl);
        break;
    case input::Key::left:
        buffer_.move_left(e.shift, e.ctrl);
        break;
    case input::Key::right:
        buffer_.move_right(e.shift, e.ctrl);
        break;
    case input::Key::up:
    case input::Key::down:
        buffer_.move_vertical(line_map_, Geometry(*this), e.key == input::Key::up ? -1 : 1, e.shift);
        break;
    case input::Key::home:
        buffer_.move_home(line_map_, e.shift, e.ctrl);
        break;
    case input::Key::end:
        buffer_.move_end(line_map_, e.shift, e.ctrl);
        break;
    case input::Key::enter:
        if (options_.multiline && !e.ctrl) {
            buffer_.type(U"\n");
        } else {
            submit = true;
        }
        break;
    case input::Key::escape:
        set_focused(false);
        return false;
    case input::Key::tab:
        if (!options_.accepts_tab || e.shift || e.ctrl) {
            return false;
        }
        if (accepted(U"\t").empty()) {
            break;
        }
        buffer_.insert(U"\t");
        break;
    default:
        switch (shortcut(e)) {
        case Shortcut::none:
            return false;
        case Shortcut::select_all:
            buffer_.select_all();
            break;
        case Shortcut::copy:
            copy_selection();
            break;
        case Shortcut::cut:
            copy_selection();
            if (buffer_.has_selection()) {
                buffer_.erase_selection();
            }
            break;
        case Shortcut::paste:
            if (options_.clipboard) {
                buffer_.insert(accepted(text::decode_utf8(options_.clipboard->text())));
            }
            break;
        case Shortcut::undo:
            if (!buffer_.undo()) {
                return false;
            }
            break;
        case Shortcut::redo:
            if (!buffer_.redo()) {
                return false;
            }
            break;
        }
        break;
    }
    if (e.key != input::Key::up && e.key != input::Key::down) {
        buffer_.reset_goal();
    }
    relayout();
    reset_blink();
    return submit;
}

void TextField::handle(const input::TextEvent& e) {
    if (!focused_) {
        return;
    }
    if (e.kind == input::TextEventKind::commit) {
        buffer_.commit(accepted(text::decode_utf8(e.text)));
    } else {
        buffer_.compose(text::decode_utf8(e.text), e.cursor);
    }
    buffer_.reset_goal();
    relayout();
    reset_blink();
}

void TextField::cancel_interaction() {
    press_ = {};
    hover_.animate_to(0.0);
    dragging_ = false;
}

void TextField::step(double dt) {
    blink_clock_ += dt;
    buffer_.advance(dt);
    label_progress_.step(dt);
    focus_.step(dt);
    hover_.step(dt);
    placeholder_opacity_.step(dt);
    enabled_.step(dt);
    error_amount_.step(dt);
}

bool TextField::animating() const {
    return !label_progress_.settled() || !focus_.settled() || !hover_.settled() || !placeholder_opacity_.settled() ||
           !enabled_.settled() || !error_amount_.settled();
}

double TextField::next_blink_seconds() const {
    if (!focused_) {
        return -1.0;
    }
    return t.caret_blink_seconds - std::fmod(blink_clock_, t.caret_blink_seconds);
}

render::Rect TextField::caret_area() const {
    const std::size_t caret = buffer_.caret();
    const std::size_t k = line_map_.line_of(caret, !buffer_.composing() && buffer_.upstream());
    const double x = text_left() + x_in_line(k, caret) - scroll_x_;
    return {x, input_top() + line_tops_[k], t.caret_width, line_tops_[k + 1] - line_tops_[k]};
}

void TextField::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.corner, r.width, r.height);
    const double f = focus_.value();
    const double outline_width = t.outline_width + (t.focus_outline_width - t.outline_width) * f;
    const color::Argb idle_outline = color::mix(scheme[t.outline], scheme[t.hover_outline], hover_.value());
    const double e = std::clamp(error_amount_.value(), 0.0, 1.0);
    const color::Argb error_outline = color::mix(scheme[t.error], scheme[t.error_hover_outline], hover_.value() * (1.0 - f));
    const color::Argb outline = enabled_.mix(resolve(scheme, t.disabled_outline),
                                             color::mix(color::mix(idle_outline, scheme[t.focus_outline], f), error_outline, e));

    const double p = std::clamp(label_progress_.value(), 0.0, 1.0);
    const double small_scale = label_small_.style.size / label_large_.style.size;
    const double scale = 1.0 + (small_scale - 1.0) * p;
    if (p > 0.0 && !options_.label.empty()) {
        const double gap_left = text_left() - t.label_gap_padding;
        const double gap_right = gap_left + label_large_.width * scale + 2 * t.label_gap_padding;
        canvas.fill_path(notched_ring_path(r, radius, outline_width, gap_left, gap_right), outline, render::FillRule::even_odd);
    } else {
        canvas.fill_path(render::ring_path(r, radius, outline_width), outline, render::FillRule::even_odd);
    }

    const double rest_top = r.y + (t.height - label_large_.box_height()) / 2.0;
    const double float_top = r.y - label_large_.box_height() * scale / 2.0;
    const double label_top = rest_top + (float_top - rest_top) * p;
    const color::Argb idle_label = color::mix(scheme[t.label], scheme[t.hover_label], hover_.value());
    const color::Argb label_color =
        enabled_.mix(resolve(scheme, t.disabled_label), color::mix(color::mix(idle_label, scheme[t.focus_label], f), scheme[t.error], e));
    canvas.save();
    canvas.translate(text_left(), label_top);
    canvas.scale(scale);
    text::draw_line(canvas, fonts_, label_large_, {0.0, 0.0}, label_color);
    canvas.restore();

    if (!options_.supporting_text.empty()) {
        const color::Argb supporting =
            enabled_.mix(resolve(scheme, t.disabled_supporting), color::mix(scheme[t.supporting], scheme[t.error], e));
        text::draw_line(canvas, fonts_, supporting_, {text_left(), r.y + r.height + t.supporting_top_padding}, supporting);
    }

    canvas.push_clip({text_left() - t.caret_width, r.y, inner_width() + 2 * t.caret_width, r.height});
    const double left = text_left() - scroll_x_;
    const double top = input_top();
    const color::Argb input_color = enabled_.mix(resolve(scheme, t.disabled_input), scheme[t.input]);

    if (placeholder_shown()) {
        const color::Argb placeholder = enabled_.mix(resolve(scheme, t.disabled_placeholder), scheme[t.placeholder]);
        text::draw_line(canvas, fonts_, placeholder_, {text_left(), top},
                        color::scale_alpha(placeholder, std::clamp(placeholder_opacity_.value(), 0.0, 1.0)));
    }

    if (focused_ && buffer_.has_selection()) {
        const color::Argb selection = color::with_alpha(scheme[t.selection], t.selection_opacity);
        const std::size_t first = buffer_.shown_index(buffer_.selection_start());
        const std::size_t last = buffer_.shown_index(buffer_.selection_end());
        for (std::size_t k = 0; k < lines_.size(); ++k) {
            const std::size_t a = std::max(first, lines_[k].first);
            const std::size_t b = std::min(last, line_map_.limit(k));
            if (a >= b) {
                continue;
            }
            const double x0 = x_in_line(k, a);
            const double x1 = x_in_line(k, b);
            canvas.fill_rect({left + x0, top + line_tops_[k], x1 - x0, line_tops_[k + 1] - line_tops_[k]}, selection);
        }
    }

    for (std::size_t k = 0; k < lines_.size(); ++k) {
        text::draw_line(canvas, fonts_, lines_[k], {left, top + line_tops_[k]}, input_color);
    }

    const std::size_t composition_first = buffer_.composition_first();
    const std::size_t composition_end = composition_first + buffer_.composition().size();
    for (std::size_t k = 0; buffer_.composing() && k < lines_.size(); ++k) {
        const std::size_t a = std::max(composition_first, lines_[k].first);
        const std::size_t b = std::min(composition_end, line_map_.limit(k));
        if (a >= b) {
            continue;
        }
        const double x0 = x_in_line(k, a);
        const double x1 = x_in_line(k, b);
        canvas.fill_rect({left + x0, top + line_tops_[k] + lines_[k].baseline + t.composition_underline * 2, x1 - x0,
                          t.composition_underline},
                         input_color);
    }

    const bool caret_on = std::fmod(blink_clock_, 2 * t.caret_blink_seconds) < t.caret_blink_seconds;
    if (focused_ && caret_on) {
        const render::Rect c = caret_area();
        canvas.fill_rect({c.x - t.caret_width / 2.0, c.y, t.caret_width, c.height}, color::mix(scheme[t.caret], scheme[t.error], e));
    }
    canvas.pop_clip();
}

}
