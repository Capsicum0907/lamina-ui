#pragma once

#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/input.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"

namespace lamina::widgets {

class TextField {
public:
    TextField(std::string label, const text::FontStack& fonts, double width);

    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    bool focused() const { return focused_; }
    void set_focused(bool focused);

    std::string text() const;
    void set_text(std::string_view utf8);

    void handle(const platform::PointerEvent& event);
    bool handle(const platform::KeyEvent& event);
    void handle(const platform::TextEvent& event);

    void step(double dt_seconds);
    bool animating() const;
    double next_blink_seconds() const;
    render::Rect caret_area() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, color::Argb background) const;

private:
    void relayout();
    void reset_blink();
    double text_left() const;
    double input_top() const;

    std::string label_text_;
    const text::FontStack& fonts_;
    double width_;
    render::Point origin_{0.0, 0.0};

    std::u32string text_;
    std::size_t cursor_ = 0;
    std::u32string composition_;
    std::size_t composition_cursor_ = 0;
    text::TextLine line_;
    std::size_t composition_first_ = 0;

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
