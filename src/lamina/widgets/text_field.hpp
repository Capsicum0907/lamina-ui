#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/edit_buffer.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/text/line_map.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

using CharacterFilter = std::function<std::optional<char32_t>(char32_t)>;

std::optional<char32_t> ascii_digit(char32_t c);

struct TextFieldOptions {
    std::string label;
    std::string placeholder = {};
    bool multiline = false;
    bool accepts_tab = false;
    input::Clipboard* clipboard = nullptr;
    bool enabled = true;
    std::string accessible_name = {};
    std::string supporting_text = {};
    CharacterFilter filter = {};
    std::size_t max_length = 0;
    std::string mask = {};
    double min_width = tokens::outlined_text_field.min_width;
    bool outlined = true;
    std::optional<double> padding_start = std::nullopt;
    std::optional<double> padding_end = std::nullopt;
};

class TextField {
public:
    TextField(TextFieldOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    double container_height() const;
    void set_width(double width);
    void set_origin(render::Point origin) { origin_ = origin; }
    layout::Size measure(const layout::Constraints&) const { return {width_, height()}; }
    void arrange(const render::Rect& r) { origin_ = {r.x, r.y}; }
    render::Rect bounds() const;
    bool focused() const { return focused_; }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    const std::string& accessible_name() const {
        return !options_.accessible_name.empty() ? options_.accessible_name : !options_.label.empty() ? options_.label : options_.placeholder;
    }
    AccessibleNode accessible() const {
        return {AccessibleRole::text_field, accessible_name(), bounds(),
                {.enabled = enabled(), .focusable = enabled(), .focused = focused(), .invalid = error_},
                text(), !options_.supporting_text.empty() ? options_.supporting_text : options_.label.empty() ? std::string() : options_.placeholder};
    }
    const std::string& supporting_text() const { return options_.supporting_text; }
    void set_supporting_text(std::string text);
    bool error() const { return error_; }
    void set_error(bool error);
    std::string display_text() const;
    double placeholder_opacity() const { return placeholder_opacity_.value(); }
    bool placeholder_shown() const;
    bool multiline() const { return options_.multiline; }
    bool accepts_tab() const { return options_.accepts_tab; }
    void set_clipboard(input::Clipboard* clipboard) { options_.clipboard = clipboard; }
    void set_padding(std::optional<double> start, std::optional<double> end);

    std::string text() const;
    void set_text(std::string_view utf8);
    std::size_t cursor() const { return buffer_.cursor(); }
    std::size_t anchor() const { return buffer_.anchor(); }
    void select(std::size_t anchor, std::size_t cursor);
    std::string selected_text() const;
    std::size_t line_count() const { return lines_.size(); }
    bool can_undo() const { return buffer_.can_undo(); }
    bool can_redo() const { return buffer_.can_redo(); }
    bool handles_history_key(const input::KeyEvent& event) const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void handle(const input::TextEvent& event);
    void cancel_interaction();

    void step(double dt_seconds);
    bool animating() const;
    double next_blink_seconds() const;
    render::Rect caret_area() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Hit {
        std::size_t index;
        bool upstream;
    };

    class Geometry final : public text::CaretGeometry {
    public:
        explicit Geometry(const TextField& field) : field_(field) {}
        double x_at(std::size_t line, std::size_t index) const override { return field_.x_in_line(line, index); }
        std::size_t index_at(std::size_t line, double x) const override;

    private:
        const TextField& field_;
    };

    enum class InputPhase { focused, unfocused_empty, unfocused_not_empty };

    InputPhase input_phase() const;
    void update_phase();
    void relayout();
    void reset_blink();
    void update_scroll();
    double text_left() const;
    double inner_width() const;
    double padding_start() const;
    double padding_end() const;
    double input_top() const;
    double content_height() const;

    void copy_selection() const;
    std::u32string accepted(std::u32string_view text) const;
    std::size_t length_limit() const;
    std::size_t to_display(std::size_t raw) const;
    std::size_t to_raw(std::size_t display) const;
    bool masked() const { return !options_.mask.empty(); }
    double x_in_line(std::size_t line, std::size_t shown) const;
    Hit hit(double x, double y) const;

    TextFieldOptions options_;
    text::FontStack fonts_;
    double width_;
    render::Point origin_{0.0, 0.0};

    text::EditBuffer buffer_;
    std::vector<text::TextLine> lines_;
    text::LineMap line_map_;
    std::vector<double> line_tops_;
    double scroll_x_ = 0.0;
    bool dragging_ = false;

    text::TextLine label_large_;
    text::TextLine label_small_;
    text::TextLine placeholder_;

    bool focused_ = false;
    PressState press_;
    double blink_clock_ = 0.0;
    motion::Animated label_progress_;
    motion::Animated focus_;
    motion::Animated hover_;
    InputPhase phase_ = InputPhase::unfocused_empty;
    motion::Animated placeholder_opacity_;
    Enablement enabled_;
    text::TextLine supporting_;
    bool error_ = false;
    motion::Animated error_amount_;
};

}
