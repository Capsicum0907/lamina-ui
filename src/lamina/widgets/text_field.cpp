#include "lamina/widgets/text_field.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/text_field.hpp"

namespace lamina::widgets {

namespace {

const auto& t = tokens::outlined_text_field;

tokens::SpringSpec label_spring() {
    return tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
}

tokens::SpringSpec effects_spring() {
    return tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
}

text::TextStyle style_of(tokens::TypeRole role) {
    return text::text_style(tokens::type_style(role));
}

bool inside(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

double line_box(const text::TextLine& line) {
    return std::max(line.style.line_height, line.ascent + line.descent);
}

}

TextField::TextField(std::string label, const text::FontStack& fonts, double width)
    : label_text_(std::move(label)),
      fonts_(fonts),
      width_(std::max(width, t.min_width)),
      label_large_(text::layout_line(fonts, label_text_, style_of(t.label_type))),
      label_small_(text::layout_line(fonts, label_text_, style_of(t.minimized_label_type))),
      label_progress_(0.0, label_spring()),
      focus_(0.0, effects_spring()),
      hover_(0.0, effects_spring()) {
    relayout();
}

render::Rect TextField::bounds() const {
    return {origin_.x, origin_.y, width_, t.height};
}

double TextField::text_left() const {
    return origin_.x + t.padding;
}

double TextField::input_top() const {
    return origin_.y + (t.height - line_box(line_)) / 2.0;
}

std::string TextField::text() const {
    return text::encode_utf8(text_);
}

void TextField::set_text(std::string_view utf8) {
    text_ = text::decode_utf8(utf8);
    cursor_ = text_.size();
    composition_.clear();
    relayout();
}

void TextField::set_focused(bool focused) {
    if (focused_ == focused) {
        return;
    }
    focused_ = focused;
    if (!focused_) {
        composition_.clear();
        relayout();
    }
    reset_blink();
    focus_.animate_to(focused_ ? 1.0 : 0.0);
    label_progress_.animate_to(focused_ || !text_.empty() ? 1.0 : 0.0);
}

void TextField::relayout() {
    std::u32string shown = text_.substr(0, cursor_) + composition_ + text_.substr(cursor_);
    composition_first_ = cursor_;
    line_ = text::layout_line(fonts_, text::encode_utf8(shown), style_of(t.input_type));
    label_progress_.animate_to(focused_ || !text_.empty() || !composition_.empty() ? 1.0 : 0.0);
}

void TextField::reset_blink() {
    blink_clock_ = 0.0;
}

void TextField::handle(const platform::PointerEvent& e) {
    const bool over = e.action != platform::PointerAction::leave && inside(bounds(), e.x_dp, e.y_dp);
    hovered_ = over;
    hover_.animate_to(over ? 1.0 : 0.0);
    if (e.action != platform::PointerAction::down) {
        return;
    }
    if (!over) {
        set_focused(false);
        return;
    }
    set_focused(true);
    if (composition_.empty()) {
        cursor_ = std::min(text::index_at(line_, e.x_dp - text_left()), text_.size());
        relayout();
    }
    reset_blink();
}

bool TextField::handle(const platform::KeyEvent& e) {
    if (!focused_ || !composition_.empty()) {
        return false;
    }
    switch (e.key) {
    case platform::Key::backspace:
        if (cursor_ > 0) {
            text_.erase(cursor_ - 1, 1);
            --cursor_;
        }
        break;
    case platform::Key::delete_forward:
        if (cursor_ < text_.size()) {
            text_.erase(cursor_, 1);
        }
        break;
    case platform::Key::left:
        cursor_ = cursor_ > 0 ? cursor_ - 1 : 0;
        break;
    case platform::Key::right:
        cursor_ = std::min(cursor_ + 1, text_.size());
        break;
    case platform::Key::home:
        cursor_ = 0;
        break;
    case platform::Key::end:
        cursor_ = text_.size();
        break;
    case platform::Key::escape:
        set_focused(false);
        return false;
    case platform::Key::enter:
        return true;
    case platform::Key::tab:
        return false;
    }
    relayout();
    reset_blink();
    return false;
}

void TextField::handle(const platform::TextEvent& e) {
    if (!focused_) {
        return;
    }
    if (e.kind == platform::TextEventKind::commit) {
        const std::u32string inserted = text::decode_utf8(e.text);
        composition_.clear();
        text_.insert(cursor_, inserted);
        cursor_ += inserted.size();
    } else {
        composition_ = text::decode_utf8(e.text);
        composition_cursor_ = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, e.cursor)), 0, composition_.size());
    }
    relayout();
    reset_blink();
}

void TextField::step(double dt) {
    blink_clock_ += dt;
    label_progress_.step(dt);
    focus_.step(dt);
    hover_.step(dt);
}

bool TextField::animating() const {
    return !label_progress_.settled() || !focus_.settled() || !hover_.settled();
}

double TextField::next_blink_seconds() const {
    if (!focused_) {
        return -1.0;
    }
    return t.caret_blink_seconds - std::fmod(blink_clock_, t.caret_blink_seconds);
}

render::Rect TextField::caret_area() const {
    const std::size_t caret = composition_first_ + (composition_.empty() ? 0 : composition_cursor_);
    const double x = text_left() + text::caret_x(line_, composition_.empty() ? cursor_ : caret);
    return {x, input_top(), t.caret_width, line_box(line_)};
}

void TextField::paint(render::Canvas& canvas, const color::ColorScheme& scheme, color::Argb background) const {
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(t.corner, r.width, r.height);
    const double f = focus_.value();
    const double outline_width = t.outline_width + (t.focus_outline_width - t.outline_width) * f;
    const color::Argb idle_outline = color::mix(scheme[t.outline], scheme[t.hover_outline], hover_.value());
    const color::Argb outline = color::mix(idle_outline, scheme[t.focus_outline], f);

    render::Path ring = render::rounded_rect_path(r, render::uniform_radii(radius));
    const render::Rect inner{r.x + outline_width, r.y + outline_width, r.width - 2 * outline_width, r.height - 2 * outline_width};
    ring.append(render::rounded_rect_path(inner, render::uniform_radii(std::max(0.0, radius - outline_width))));
    canvas.fill_path(ring, outline, render::FillRule::even_odd);

    const double p = std::clamp(label_progress_.value(), 0.0, 1.0);
    const double small_scale = label_small_.style.size / label_large_.style.size;
    const double scale = 1.0 + (small_scale - 1.0) * p;
    if (p > 0.0) {
        const double gap = (label_large_.width * scale + 2 * t.label_gap_padding) ;
        canvas.fill_rounded_rect({text_left() - t.label_gap_padding, r.y - t.focus_outline_width, gap, 2 * t.focus_outline_width},
                                 render::uniform_radii(0.0), background);
    }

    const double rest_top = r.y + (r.height - line_box(label_large_)) / 2.0;
    const double float_top = r.y - line_box(label_large_) * scale / 2.0;
    const double label_top = rest_top + (float_top - rest_top) * p;
    const color::Argb idle_label = color::mix(scheme[t.label], scheme[t.hover_label], hover_.value());
    const color::Argb label_color = color::mix(idle_label, scheme[t.focus_label], f);
    canvas.save();
    canvas.translate(text_left(), label_top);
    canvas.scale(scale);
    text::draw_line(canvas, fonts_, label_large_, {0.0, 0.0}, label_color);
    canvas.restore();

    const render::Point at{text_left(), input_top()};
    text::draw_line(canvas, fonts_, line_, at, scheme[t.input]);

    if (!composition_.empty()) {
        const double x0 = text::caret_x(line_, composition_first_);
        const double x1 = text::caret_x(line_, composition_first_ + composition_.size());
        canvas.fill_rounded_rect({at.x + x0, at.y + line_.baseline + t.composition_underline * 2, x1 - x0, t.composition_underline},
                                 render::uniform_radii(0.0), scheme[t.input]);
    }

    const bool caret_on = std::fmod(blink_clock_, 2 * t.caret_blink_seconds) < t.caret_blink_seconds;
    if (focused_ && caret_on) {
        const render::Rect c = caret_area();
        canvas.fill_rounded_rect({c.x - t.caret_width / 2.0, c.y, t.caret_width, c.height}, render::uniform_radii(0.0), scheme[t.caret]);
    }
}

}
