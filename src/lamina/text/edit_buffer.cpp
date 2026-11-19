#include "lamina/text/edit_buffer.hpp"

#include <utility>

#include "lamina/text/word_boundary.hpp"

namespace lamina::text {

namespace {

constexpr char32_t first_printable = U' ';
constexpr char32_t delete_char = U'\x7f';

bool is_space(char32_t c) {
    return c == U' ' || c == U'\t' || c == U'\n';
}

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
    composition_before_.reset();
    history_.clear();
    sync();
}

EditState EditBuffer::state() const {
    return {text_, anchor_, cursor_};
}

void EditBuffer::restore(const EditState& saved) {
    text_ = saved.text;
    anchor_ = std::min(saved.anchor, text_.size());
    cursor_ = std::min(saved.cursor, text_.size());
    upstream_ = false;
    goal_x_.reset();
    sync();
}

void EditBuffer::record(EditState before, EditGroup group, bool starts_step) {
    if (before.text != text_) {
        history_.record(std::move(before), state(), group, starts_step);
    }
}

void EditBuffer::place(std::size_t index) {
    cursor_ = std::min(index, text_.size());
    anchor_ = cursor_;
    upstream_ = false;
    sync();
}

void EditBuffer::select(std::size_t anchor, std::size_t cursor) {
    anchor_ = std::min(anchor, text_.size());
    cursor_ = std::min(cursor, text_.size());
    upstream_ = false;
    goal_x_.reset();
    history_.seal();
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
    history_.seal();
    const TextRange word = word_at(text_, index);
    anchor_ = word.first;
    cursor_ = word.last;
    upstream_ = false;
    sync();
}

void EditBuffer::move_to(std::size_t index, bool extend, bool upstream) {
    history_.seal();
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
    place(first + inserted.size());
}

void EditBuffer::erase(std::size_t first, std::size_t last) {
    EditState before = state();
    text_.erase(first, last - first);
    place(first);
    record(std::move(before), EditGroup::deleting);
}

void EditBuffer::insert(std::u32string_view text) {
    EditState before = state();
    replace_selection(normalize(text));
    record(std::move(before), EditGroup::single);
}

void EditBuffer::type(std::u32string_view text) {
    const std::u32string typed = normalize(text);
    if (typed.size() != 1 || has_selection()) {
        insert(typed);
        return;
    }
    const bool starts_step = is_space(typed.front()) && cursor_ > 0 && !is_space(text_[cursor_ - 1]);
    EditState before = state();
    replace_selection(typed);
    record(std::move(before), EditGroup::typing, starts_step);
}

void EditBuffer::erase_selection() {
    EditState before = state();
    replace_selection({});
    record(std::move(before), EditGroup::single);
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
    if (!text.empty() && !composition_before_) {
        composition_before_ = state();
    }
    if (!text.empty() && has_selection()) {
        replace_selection({});
    }
    composition_ = text;
    composition_cursor_ = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, cursor)), 0, composition_.size());
    if (composition_.empty()) {
        finish_composition();
    }
    sync();
}

void EditBuffer::commit(std::u32string_view text) {
    composition_.clear();
    if (!composition_before_) {
        type(text);
        return;
    }
    EditState before = std::move(*composition_before_);
    composition_before_.reset();
    replace_selection(normalize(text));
    record(std::move(before), EditGroup::single);
}

void EditBuffer::finish_composition() {
    if (composition_before_) {
        EditState before = std::move(*composition_before_);
        composition_before_.reset();
        record(std::move(before), EditGroup::single);
    }
}

void EditBuffer::clear_composition() {
    composition_.clear();
    finish_composition();
    sync();
}

bool EditBuffer::undo() {
    if (composing()) {
        return false;
    }
    const std::optional<EditState> target = history_.undo();
    if (!target) {
        return false;
    }
    restore(*target);
    return true;
}

bool EditBuffer::redo() {
    if (composing()) {
        return false;
    }
    const std::optional<EditState> target = history_.redo();
    if (!target) {
        return false;
    }
    restore(*target);
    return true;
}

}
