#include "lamina/widgets/text_field.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/text/line_break.hpp"
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

enum class WordClass { space, newline, punctuation, hiragana, katakana, han, word };

struct ClassRange {
    char32_t first;
    char32_t last;
    WordClass cls;
};

constexpr ClassRange class_ranges[] = {
    {U'々', U'々', WordClass::han},
    {U'　', U'〿', WordClass::punctuation},
    {U'ぁ', U'ゟ', WordClass::hiragana},
    {U'゠', U'ヿ', WordClass::katakana},
    {U'ㇰ', U'ㇿ', WordClass::katakana},
    {U'㐀', U'䶿', WordClass::han},
    {U'一', U'鿿', WordClass::han},
    {U'豈', U'﫿', WordClass::han},
    {U'\U00020000', U'\U0003134f', WordClass::han},
    {U' ', U'⁯', WordClass::punctuation},
    {U'！', U'／', WordClass::punctuation},
    {U'：', U'＠', WordClass::punctuation},
    {U'［', U'｀', WordClass::punctuation},
    {U'｛', U'･', WordClass::punctuation},
    {U'ｦ', U'ﾟ', WordClass::katakana},
};

constexpr char32_t ascii_end = U'\x80';
constexpr char32_t first_printable = U' ';
constexpr char32_t delete_char = U'\x7f';

WordClass word_class(char32_t c) {
    if (c == U'\n') {
        return WordClass::newline;
    }
    if (text::is_space(c)) {
        return WordClass::space;
    }
    for (const ClassRange& r : class_ranges) {
        if (c >= r.first && c <= r.last) {
            return r.cls;
        }
    }
    if (c < ascii_end) {
        const bool alnum = (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_';
        return alnum ? WordClass::word : WordClass::punctuation;
    }
    return WordClass::word;
}

std::size_t next_word(std::u32string_view s, std::size_t i) {
    if (i >= s.size()) {
        return s.size();
    }
    const WordClass c = word_class(s[i]);
    if (c != WordClass::space) {
        while (i < s.size() && word_class(s[i]) == c) {
            ++i;
        }
    }
    while (i < s.size() && word_class(s[i]) == WordClass::space) {
        ++i;
    }
    return i;
}

std::size_t previous_word(std::u32string_view s, std::size_t i) {
    i = std::min(i, s.size());
    while (i > 0 && word_class(s[i - 1]) == WordClass::space) {
        --i;
    }
    if (i == 0) {
        return 0;
    }
    const WordClass c = word_class(s[i - 1]);
    while (i > 0 && word_class(s[i - 1]) == c) {
        --i;
    }
    return i;
}

bool is_letter_key(input::Key k) {
    return k == input::Key::a || k == input::Key::c || k == input::Key::v || k == input::Key::x;
}

}

TextField::TextField(std::string label, const text::FontStack& fonts, double width, TextFieldOptions options)
    : label_text_(std::move(label)),
      fonts_(fonts),
      width_(std::max(width, t.min_width)),
      options_(options),
      label_large_(text::layout_line(fonts, label_text_, style_of(t.label_type))),
      label_small_(text::layout_line(fonts, label_text_, style_of(t.minimized_label_type))),
      label_progress_(0.0, label_spring()),
      focus_(0.0, effects_spring()),
      hover_(0.0, effects_spring()) {
    relayout();
}

render::Rect TextField::bounds() const {
    const double height = options_.multiline ? std::max(t.height, content_height() + 2 * t.vertical_padding) : t.height;
    return {origin_.x, origin_.y, width_, height};
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
    return origin_.y + (bounds().height - content_height()) / 2.0;
}

std::string TextField::text() const {
    return text::encode_utf8(text_);
}

void TextField::set_text(std::string_view utf8) {
    text_ = normalize(text::decode_utf8(utf8));
    cursor_ = text_.size();
    anchor_ = cursor_;
    upstream_ = false;
    goal_x_.reset();
    composition_.clear();
    relayout();
}

void TextField::select(std::size_t anchor, std::size_t cursor) {
    anchor_ = std::min(anchor, text_.size());
    cursor_ = std::min(cursor, text_.size());
    upstream_ = false;
    goal_x_.reset();
    relayout();
}

std::string TextField::selected_text() const {
    return text::encode_utf8(std::u32string_view(text_).substr(selection_start(), selection_end() - selection_start()));
}

void TextField::set_focused(bool focused) {
    if (focused_ == focused) {
        return;
    }
    focused_ = focused;
    dragging_ = false;
    if (!focused_) {
        composition_.clear();
        relayout();
    }
    reset_blink();
    focus_.animate_to(focused_ ? 1.0 : 0.0);
    label_progress_.animate_to(focused_ || !text_.empty() ? 1.0 : 0.0);
}

std::u32string TextField::normalize(std::u32string_view input) const {
    std::u32string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        char32_t c = input[i];
        if (c == U'\r') {
            if (i + 1 < input.size() && input[i + 1] == U'\n') {
                continue;
            }
            c = U'\n';
        }
        if (c == U'\n') {
            out.push_back(options_.multiline ? U'\n' : U' ');
        } else if (c == U'\t') {
            out.push_back(U' ');
        } else if (c >= first_printable && c != delete_char) {
            out.push_back(c);
        }
    }
    return out;
}

void TextField::relayout() {
    shown_ = text_.substr(0, cursor_) + composition_ + text_.substr(cursor_);
    composition_first_ = cursor_;
    const text::TextStyle style = style_of(t.input_type);
    const std::string utf8 = text::encode_utf8(shown_);
    if (options_.multiline) {
        lines_ = text::layout_paragraph(fonts_, utf8, style, inner_width()).lines;
    } else {
        lines_.assign(1, text::layout_line(fonts_, utf8, style));
    }
    line_tops_.assign(1, 0.0);
    for (const text::TextLine& line : lines_) {
        line_tops_.push_back(line_tops_.back() + line_box(line));
    }
    update_scroll();
    label_progress_.animate_to(focused_ || !text_.empty() || !composition_.empty() ? 1.0 : 0.0);
}

void TextField::update_scroll() {
    if (options_.multiline) {
        scroll_x_ = 0.0;
        return;
    }
    const std::size_t caret = caret_index();
    const double x = x_in_line(line_of(caret, false), caret);
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

std::size_t TextField::shown_index(std::size_t index) const {
    return index < composition_first_ ? index : index + composition_.size();
}

std::size_t TextField::caret_index() const {
    return composition_.empty() ? cursor_ : composition_first_ + composition_cursor_;
}

std::size_t TextField::line_limit(std::size_t line) const {
    std::size_t end = line + 1 < lines_.size() ? lines_[line + 1].first : shown_.size();
    if (end > lines_[line].last && shown_[end - 1] == U'\n') {
        --end;
    }
    return end;
}

bool TextField::wraps_at(std::size_t line, std::size_t shown) const {
    return line + 1 < lines_.size() && lines_[line + 1].first == shown && line_limit(line) == shown;
}

std::size_t TextField::line_of(std::size_t shown, bool upstream) const {
    std::size_t k = 0;
    while (k + 1 < lines_.size() && lines_[k + 1].first <= shown) {
        ++k;
    }
    if (upstream && k > 0 && wraps_at(k - 1, shown)) {
        --k;
    }
    return k;
}

double TextField::x_in_line(std::size_t line, std::size_t shown) const {
    const text::TextLine& l = lines_[line];
    if (shown <= l.last) {
        return text::caret_x(l, shown);
    }
    const std::size_t end = std::min(shown, line_limit(line));
    const std::u32string_view run = std::u32string_view(shown_).substr(l.first, end - l.first);
    return text::layout_line(fonts_, text::encode_utf8(run), l.style).width;
}

TextField::Hit TextField::hit(double x, double y) const {
    const double local_y = y - input_top();
    std::size_t k = 0;
    while (k + 1 < lines_.size() && local_y >= line_tops_[k + 1]) {
        ++k;
    }
    const std::size_t index = text::index_at(lines_[k], x - text_left() + scroll_x_);
    return {index, wraps_at(k, index)};
}

TextField::Hit TextField::vertical(int direction) {
    const std::size_t k = line_of(cursor_, upstream_);
    if (!goal_x_) {
        goal_x_ = x_in_line(k, cursor_);
    }
    if (direction < 0 && k == 0) {
        return {0, false};
    }
    if (direction > 0 && k + 1 >= lines_.size()) {
        return {text_.size(), false};
    }
    const std::size_t target = direction < 0 ? k - 1 : k + 1;
    const std::size_t index = text::index_at(lines_[target], *goal_x_);
    return {index, wraps_at(target, index)};
}

void TextField::move_to(std::size_t index, bool extend, bool upstream) {
    cursor_ = std::min(index, text_.size());
    if (!extend) {
        anchor_ = cursor_;
    }
    upstream_ = upstream;
}

void TextField::replace_selection(std::u32string_view inserted) {
    const std::size_t first = selection_start();
    text_.replace(first, selection_end() - first, inserted);
    move_to(first + inserted.size(), false);
}

void TextField::erase(std::size_t first, std::size_t last) {
    text_.erase(first, last - first);
    move_to(first, false);
}

void TextField::copy_selection() const {
    if (options_.clipboard && has_selection()) {
        options_.clipboard->set_text(selected_text());
    }
}

void TextField::select_word_at(std::size_t index) {
    if (text_.empty()) {
        move_to(0, false);
        return;
    }
    const std::size_t at = std::min(index, text_.size() - 1);
    const WordClass c = word_class(text_[at]);
    std::size_t first = at;
    while (first > 0 && word_class(text_[first - 1]) == c) {
        --first;
    }
    std::size_t last = at + 1;
    while (last < text_.size() && word_class(text_[last]) == c) {
        ++last;
    }
    anchor_ = first;
    cursor_ = last;
    upstream_ = false;
}

void TextField::handle(const input::PointerEvent& e) {
    const bool over = e.action != input::PointerAction::leave && inside(bounds(), e.x_dp, e.y_dp);
    hovered_ = over;
    hover_.animate_to(over ? 1.0 : 0.0);
    if (e.action == input::PointerAction::up) {
        dragging_ = false;
        return;
    }
    if (e.action == input::PointerAction::move) {
        if (dragging_ && composition_.empty()) {
            const Hit h = hit(e.x_dp, e.y_dp);
            move_to(h.index, true, h.upstream);
            relayout();
            reset_blink();
        }
        return;
    }
    if (e.action != input::PointerAction::down) {
        return;
    }
    if (!over) {
        set_focused(false);
        return;
    }
    set_focused(true);
    if (composition_.empty()) {
        const Hit h = hit(e.x_dp, e.y_dp);
        if (e.clicks == 2) {
            select_word_at(h.index);
        } else if (e.clicks >= 3) {
            anchor_ = 0;
            move_to(text_.size(), true);
        } else {
            move_to(h.index, false, h.upstream);
        }
        dragging_ = e.clicks <= 1;
        goal_x_.reset();
        relayout();
    }
    reset_blink();
}

bool TextField::handle(const input::KeyEvent& e) {
    if (!focused_ || !composition_.empty()) {
        return false;
    }
    if (is_letter_key(e.key) && !e.ctrl) {
        return false;
    }
    bool submit = false;
    switch (e.key) {
    case input::Key::backspace:
        if (has_selection()) {
            replace_selection({});
        } else if (cursor_ > 0) {
            erase(e.ctrl ? previous_word(text_, cursor_) : cursor_ - 1, cursor_);
        }
        break;
    case input::Key::delete_forward:
        if (has_selection()) {
            replace_selection({});
        } else if (cursor_ < text_.size()) {
            erase(cursor_, e.ctrl ? next_word(text_, cursor_) : cursor_ + 1);
        }
        break;
    case input::Key::left:
        if (has_selection() && !e.shift && !e.ctrl) {
            move_to(selection_start(), false);
        } else {
            move_to(e.ctrl ? previous_word(text_, cursor_) : (cursor_ > 0 ? cursor_ - 1 : 0), e.shift);
        }
        break;
    case input::Key::right:
        if (has_selection() && !e.shift && !e.ctrl) {
            move_to(selection_end(), false);
        } else {
            move_to(e.ctrl ? next_word(text_, cursor_) : cursor_ + 1, e.shift);
        }
        break;
    case input::Key::up:
    case input::Key::down: {
        const Hit h = vertical(e.key == input::Key::up ? -1 : 1);
        move_to(h.index, e.shift, h.upstream);
        break;
    }
    case input::Key::home:
        move_to(e.ctrl ? 0 : lines_[line_of(cursor_, upstream_)].first, e.shift);
        break;
    case input::Key::end: {
        if (e.ctrl) {
            move_to(text_.size(), e.shift);
            break;
        }
        const std::size_t k = line_of(cursor_, upstream_);
        const std::size_t limit = line_limit(k);
        move_to(limit, e.shift, wraps_at(k, limit));
        break;
    }
    case input::Key::enter:
        if (options_.multiline && !e.ctrl) {
            replace_selection(U"\n");
        } else {
            submit = true;
        }
        break;
    case input::Key::escape:
        set_focused(false);
        return false;
    case input::Key::tab:
        return false;
    case input::Key::a:
        anchor_ = 0;
        move_to(text_.size(), true);
        break;
    case input::Key::c:
        copy_selection();
        break;
    case input::Key::x:
        copy_selection();
        if (has_selection()) {
            replace_selection({});
        }
        break;
    case input::Key::v:
        if (options_.clipboard) {
            replace_selection(normalize(text::decode_utf8(options_.clipboard->text())));
        }
        break;
    }
    if (e.key != input::Key::up && e.key != input::Key::down) {
        goal_x_.reset();
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
        composition_.clear();
        replace_selection(normalize(text::decode_utf8(e.text)));
    } else {
        std::u32string composing = text::decode_utf8(e.text);
        if (!composing.empty() && has_selection()) {
            replace_selection({});
        }
        composition_ = std::move(composing);
        composition_cursor_ = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, e.cursor)), 0, composition_.size());
    }
    goal_x_.reset();
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
    const std::size_t caret = caret_index();
    const std::size_t k = line_of(caret, composition_.empty() && upstream_);
    const double x = text_left() + x_in_line(k, caret) - scroll_x_;
    return {x, input_top() + line_tops_[k], t.caret_width, line_tops_[k + 1] - line_tops_[k]};
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

    const double rest_top = r.y + (t.height - line_box(label_large_)) / 2.0;
    const double float_top = r.y - line_box(label_large_) * scale / 2.0;
    const double label_top = rest_top + (float_top - rest_top) * p;
    const color::Argb idle_label = color::mix(scheme[t.label], scheme[t.hover_label], hover_.value());
    const color::Argb label_color = color::mix(idle_label, scheme[t.focus_label], f);
    canvas.save();
    canvas.translate(text_left(), label_top);
    canvas.scale(scale);
    text::draw_line(canvas, fonts_, label_large_, {0.0, 0.0}, label_color);
    canvas.restore();

    canvas.push_clip({text_left() - t.caret_width, r.y, inner_width() + 2 * t.caret_width, r.height});
    const double left = text_left() - scroll_x_;
    const double top = input_top();

    if (focused_ && has_selection()) {
        const color::Argb selection = color::with_alpha(scheme[t.selection], t.selection_opacity);
        const std::size_t first = shown_index(selection_start());
        const std::size_t last = shown_index(selection_end());
        for (std::size_t k = 0; k < lines_.size(); ++k) {
            const std::size_t a = std::max(first, lines_[k].first);
            const std::size_t b = std::min(last, line_limit(k));
            if (a >= b) {
                continue;
            }
            const double x0 = x_in_line(k, a);
            const double x1 = x_in_line(k, b);
            canvas.fill_rounded_rect({left + x0, top + line_tops_[k], x1 - x0, line_tops_[k + 1] - line_tops_[k]},
                                     render::uniform_radii(0.0), selection);
        }
    }

    for (std::size_t k = 0; k < lines_.size(); ++k) {
        text::draw_line(canvas, fonts_, lines_[k], {left, top + line_tops_[k]}, scheme[t.input]);
    }

    for (std::size_t k = 0; !composition_.empty() && k < lines_.size(); ++k) {
        const std::size_t a = std::max(composition_first_, lines_[k].first);
        const std::size_t b = std::min(composition_first_ + composition_.size(), line_limit(k));
        if (a >= b) {
            continue;
        }
        const double x0 = x_in_line(k, a);
        const double x1 = x_in_line(k, b);
        canvas.fill_rounded_rect({left + x0, top + line_tops_[k] + lines_[k].baseline + t.composition_underline * 2, x1 - x0,
                                  t.composition_underline},
                                 render::uniform_radii(0.0), scheme[t.input]);
    }

    const bool caret_on = std::fmod(blink_clock_, 2 * t.caret_blink_seconds) < t.caret_blink_seconds;
    if (focused_ && caret_on) {
        const render::Rect c = caret_area();
        canvas.fill_rounded_rect({c.x - t.caret_width / 2.0, c.y, t.caret_width, c.height}, render::uniform_radii(0.0), scheme[t.caret]);
    }
    canvas.pop_clip();
}

}
