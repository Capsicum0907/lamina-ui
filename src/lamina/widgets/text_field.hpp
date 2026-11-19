#pragma once

#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/edit_buffer.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/text/line_map.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct TextFieldOptions {
    std::string label;
    bool multiline = false;
    input::Clipboard* clipboard = nullptr;
};

class TextField {
public:
    TextField(TextFieldOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    void set_width(double width);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    bool focused() const { return focused_; }
    void set_focused(bool focused);
    bool multiline() const { return options_.multiline; }
    void set_clipboard(input::Clipboard* clipboard) { options_.clipboard = clipboard; }

    std::string text() const;
    void set_text(std::string_view utf8);
    std::size_t cursor() const { return buffer_.cursor(); }
    std::size_t anchor() const { return buffer_.anchor(); }
    void select(std::size_t anchor, std::size_t cursor);
    std::string selected_text() const;
    std::size_t line_count() const { return lines_.size(); }

    bool handle(const input::PointerEvent& event);
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

    class Geometry final : public text::CaretGeometry {
    public:
        explicit Geometry(const TextField& field) : field_(field) {}
        double x_at(std::size_t line, std::size_t index) const override { return field_.x_in_line(line, index); }
        std::size_t index_at(std::size_t line, double x) const override;

    private:
        const TextField& field_;
    };

    void relayout();
    void reset_blink();
    void update_scroll();
    double text_left() const;
    double inner_width() const;
    double input_top() const;
    double content_height() const;

    void copy_selection() const;
    double x_in_line(std::size_t line, std::size_t shown) const;
    Hit hit(double x, double y) const;

    TextFieldOptions options_;
    const text::FontStack& fonts_;
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

    bool focused_ = false;
    PressState press_;
    double blink_clock_ = 0.0;
    motion::Animated label_progress_;
    motion::Animated focus_;
    motion::Animated hover_;
};

}
