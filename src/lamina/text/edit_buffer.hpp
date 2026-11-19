#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "lamina/text/edit_history.hpp"
#include "lamina/text/line_map.hpp"

namespace lamina::text {

struct EditBufferOptions {
    bool multiline = false;
    bool keeps_tabs = false;
    EditHistoryLimits history{};
};

class EditBuffer {
public:
    explicit EditBuffer(EditBufferOptions options = {}) : options_(options), history_(options.history) {}

    bool multiline() const { return options_.multiline; }
    bool keeps_tabs() const { return options_.keeps_tabs; }
    std::u32string_view text() const { return text_; }
    std::u32string_view shown() const { return shown_; }
    std::size_t cursor() const { return cursor_; }
    std::size_t anchor() const { return anchor_; }
    bool upstream() const { return upstream_; }

    bool has_selection() const { return cursor_ != anchor_; }
    std::size_t selection_start() const { return std::min(cursor_, anchor_); }
    std::size_t selection_end() const { return std::max(cursor_, anchor_); }
    std::u32string_view selected() const;

    bool composing() const { return !composition_.empty(); }
    std::u32string_view composition() const { return composition_; }
    std::size_t composition_first() const { return cursor_; }
    std::size_t caret() const;
    std::size_t shown_index(std::size_t index) const;

    std::u32string normalize(std::u32string_view input) const;

    void set_text(std::u32string_view text);
    void select(std::size_t anchor, std::size_t cursor);
    void select_all();
    void select_word_at(std::size_t index);
    void move_to(std::size_t index, bool extend, bool upstream = false);
    void move_left(bool extend, bool by_word);
    void move_right(bool extend, bool by_word);
    void move_home(const LineMap& lines, bool extend, bool whole_text);
    void move_end(const LineMap& lines, bool extend, bool whole_text);
    void move_vertical(const LineMap& lines, const CaretGeometry& geometry, int direction, bool extend);
    void reset_goal() { goal_x_.reset(); }

    void insert(std::u32string_view text);
    void type(std::u32string_view text);
    void erase_selection();
    void backspace(bool by_word);
    void delete_forward(bool by_word);

    void compose(std::u32string_view text, int cursor);
    void commit(std::u32string_view text);
    void clear_composition();

    bool can_undo() const { return history_.can_undo(); }
    bool can_redo() const { return history_.can_redo(); }
    bool undo();
    bool redo();
    void advance(double dt_seconds) { history_.advance(dt_seconds); }
    void close_step() { history_.seal(); }

private:
    EditState state() const;
    void restore(const EditState& saved);
    void record(EditState before, EditGroup group, bool starts_step = false);
    void place(std::size_t index);
    void replace_selection(std::u32string_view inserted);
    void erase(std::size_t first, std::size_t last);
    void finish_composition();
    void sync();

    EditBufferOptions options_;
    std::u32string text_;
    std::u32string shown_;
    std::size_t cursor_ = 0;
    std::size_t anchor_ = 0;
    bool upstream_ = false;
    std::optional<double> goal_x_;
    std::u32string composition_;
    std::size_t composition_cursor_ = 0;
    EditHistory history_;
    std::optional<EditState> composition_before_;
};

}
