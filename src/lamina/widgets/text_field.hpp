#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"

namespace lamina::widgets {

struct TextFieldOptions {
    bool multiline = false;
    input::Clipboard* clipboard = nullptr;
};

class TextField {
public:
    TextField(std::string label, const text::FontStack& fonts, double width, TextFieldOptions options = {});

    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    bool focused() const { return focused_; }
    void set_focused(bool focused);
    bool multiline() const { return options_.multiline; }
    void set_clipboard(input::Clipboard* clipboard) { options_.clipboard = clipboard; }

    std::string text() const;
    void set_text(std::string_view utf8);
    std::size_t cursor() const { return cursor_; }
    std::size_t anchor() const { return anchor_; }
    void select(std::size_t anchor, std::size_t cursor);
    std::string selected_text() const;
    std::size_t line_count() const { return lines_.size(); }

    void handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);

    void step(double dt_seconds);
    bool animating() const;
    double next_blink_seconds() const;
    render::Rect caret_area() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, color::Argb background) const;

private:
    struct Hit {
        std::size_t index;
        bool upstream;
    };

    void relayout();
    void reset_blink();
    void update_scroll();
    double text_left() const;
    double inner_width() const;
    double input_top() const;
    double content_height() const;

    bool has_selection() const { return cursor_ != anchor_; }
    std::size_t selection_start() const { return std::min(cursor_, anchor_); }
    std::size_t selection_end() const { return std::max(cursor_, anchor_); }
    void move_to(std::size_t index, bool extend, bool upstream = false);
    void replace_selection(std::u32string_view inserted);
    void erase(std::size_t first, std::size_t last);
    void copy_selection() const;
    void select_word_at(std::size_t index);
    std::u32string normalize(std::u32string_view input) const;

    std::size_t shown_index(std::size_t index) const;
    std::size_t caret_index() const;
    std::size_t line_of(std::size_t shown, bool upstream) const;
    std::size_t line_limit(std::size_t line) const;
    bool wraps_at(std::size_t line, std::size_t shown) const;
    double x_in_line(std::size_t line, std::size_t shown) const;
    Hit hit(double x, double y) const;
    Hit vertical(int direction);

    std::string label_text_;
    const text::FontStack& fonts_;
    double width_;
    TextFieldOptions options_;
    render::Point origin_{0.0, 0.0};

    std::u32string text_;
    std::size_t cursor_ = 0;
    std::size_t anchor_ = 0;
    bool upstream_ = false;
    std::optional<double> goal_x_;
    std::u32string composition_;
    std::size_t composition_cursor_ = 0;
    std::size_t composition_first_ = 0;
    std::u32string shown_;
    std::vector<text::TextLine> lines_;
    std::vector<double> line_tops_;
    double scroll_x_ = 0.0;
    bool dragging_ = false;

    text::TextLine label_large_;
    text::TextLine label_small_;

    bool focused_ = false;
    bool hovered_ = false;
    double blink_clock_ = 0.0;
    motion::Animated label_progress_;
    motion::Animated focus_;
    motion::Animated hover_;
};

}
