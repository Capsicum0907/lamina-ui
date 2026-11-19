#include "lamina/text/edit_buffer.hpp"

#include "lamina/text/word_boundary.hpp"

namespace lamina::text {

namespace {

constexpr char32_t first_printable = U' ';
constexpr char32_t delete_char = U'\x7f';

}

std::u32string_view EditBuffer::selected() const {
    return std::u32string_view(text_).substr(selection_start(), selection_end() - selection_start());
}

std::size_t EditBuffer::caret() const {
    return composition_.empty() ? cursor_ : cursor_ + composition_cursor_;
}

std::size_t EditBuffer::shown_index(std::size_t index) const {
    return index < cursor_ ? index : index + composition_.size();
}

std::u32string EditBuffer::normalize(std::u32string_view input) const {
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
            out.push_back(options_.keeps_tabs ? U'\t' : U' ');
        } else if (c >= first_printable && c != delete_char) {
            out.push_back(c);
        }
    }
    return out;
}

void EditBuffer::sync() {
    shown_ = text_.substr(0, cursor_) + composition_ + text_.substr(cursor_);
}

void EditBuffer::set_text(std::u32string_view text) {
    text_ = normalize(text);
    cursor_ = text_.size();
    anchor_ = cursor_;
    upstream_ = false;
    goal_x_.reset();
    composition_.clear();
    sync();
}

void EditBuffer::select(std::size_t anchor, std::size_t cursor) {
    anchor_ = std::min(anchor, text_.size());
    cursor_ = std::min(cursor, text_.size());
    upstream_ = false;
    goal_x_.reset();
    sync();
}

void EditBuffer::select_all() {
    anchor_ = 0;
    move_to(text_.size(), true);
}

void EditBuffer::select_word_at(std::size_t index) {
    if (text_.empty()) {
        move_to(0, false);
        return;
    }
    const TextRange word = word_at(text_, index);
    anchor_ = word.first;
    cursor_ = word.last;
    upstream_ = false;
    sync();
}

void EditBuffer::move_to(std::size_t index, bool extend, bool upstream) {
    cursor_ = std::min(index, text_.size());
    if (!extend) {
        anchor_ = cursor_;
    }
    upstream_ = upstream;
    sync();
}

void EditBuffer::move_left(bool extend, bool by_word) {
    if (has_selection() && !extend && !by_word) {
        move_to(selection_start(), false);
    } else {
        move_to(by_word ? previous_word(text_, cursor_) : (cursor_ > 0 ? cursor_ - 1 : 0), extend);
    }
}

void EditBuffer::move_right(bool extend, bool by_word) {
    if (has_selection() && !extend && !by_word) {
        move_to(selection_end(), false);
    } else {
        move_to(by_word ? next_word(text_, cursor_) : cursor_ + 1, extend);
    }
}

void EditBuffer::move_home(const LineMap& lines, bool extend, bool whole_text) {
    move_to(whole_text ? 0 : lines.first(lines.line_of(cursor_, upstream_)), extend);
}

void EditBuffer::move_end(const LineMap& lines, bool extend, bool whole_text) {
    if (whole_text) {
        move_to(text_.size(), extend);
        return;
    }
    const std::size_t k = lines.line_of(cursor_, upstream_);
    const std::size_t limit = lines.limit(k);
    move_to(limit, extend, lines.wraps_at(k, limit));
}

void EditBuffer::move_vertical(const LineMap& lines, const CaretGeometry& geometry, int direction, bool extend) {
    const std::size_t k = lines.line_of(cursor_, upstream_);
    if (!goal_x_) {
        goal_x_ = geometry.x_at(k, cursor_);
    }
    if (direction < 0 && k == 0) {
        move_to(0, extend);
        return;
    }
    if (direction > 0 && k + 1 >= lines.size()) {
        move_to(text_.size(), extend);
        return;
    }
    const std::size_t target = direction < 0 ? k - 1 : k + 1;
    const std::size_t index = geometry.index_at(target, *goal_x_);
    move_to(index, extend, lines.wraps_at(target, index));
}

void EditBuffer::replace_selection(std::u32string_view inserted) {
    const std::size_t first = selection_start();
    text_.replace(first, selection_end() - first, inserted);
    move_to(first + inserted.size(), false);
}

void EditBuffer::erase(std::size_t first, std::size_t last) {
    text_.erase(first, last - first);
    move_to(first, false);
}

void EditBuffer::insert(std::u32string_view text) {
    replace_selection(normalize(text));
}

void EditBuffer::erase_selection() {
    replace_selection({});
}

void EditBuffer::backspace(bool by_word) {
    if (has_selection()) {
        erase_selection();
    } else if (cursor_ > 0) {
        erase(by_word ? previous_word(text_, cursor_) : cursor_ - 1, cursor_);
    }
}

void EditBuffer::delete_forward(bool by_word) {
    if (has_selection()) {
        erase_selection();
    } else if (cursor_ < text_.size()) {
        erase(cursor_, by_word ? next_word(text_, cursor_) : cursor_ + 1);
    }
}

void EditBuffer::compose(std::u32string_view text, int cursor) {
    if (!text.empty() && has_selection()) {
        erase_selection();
    }
    composition_ = text;
    composition_cursor_ = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, cursor)), 0, composition_.size());
    sync();
}

void EditBuffer::commit(std::u32string_view text) {
    composition_.clear();
    insert(text);
}

void EditBuffer::clear_composition() {
    composition_.clear();
    sync();
}

}
