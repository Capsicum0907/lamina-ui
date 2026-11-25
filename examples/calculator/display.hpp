#pragma once

#include <array>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::calculator {

struct DisplayStyle {
    tokens::TypeRole focused_type = tokens::TypeRole::display_large;
    tokens::TypeRole secondary_type = tokens::TypeRole::headline_large;
    color::Role focused_color = color::Role::on_surface;
    color::Role secondary_color = color::Role::on_surface_variant;
    color::Role error_color = color::Role::error;
    tokens::TypeRole badge_type = tokens::TypeRole::label_large;
    color::Role badge_color = color::Role::on_surface_variant;
    double min_fit = 0.45;
    double line_gap = 8.0;
    double inset = 16.0;
    motion::SpringSpec size_spring = current_style().motion.default_spatial();
    motion::SpringSpec color_spring = current_style().motion.slow_effects();
};

class Display {
public:
    Display(DisplayStyle style, const text::FontStack& fonts);

    void show(const std::string& formula, const std::string& result, bool result_focused, bool error);
    void set_badge(const std::string& text, bool visible);
    void arrange(const render::Rect& rect);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Line {
        std::string text;
        text::TextLine layout;
        bool focused;
        motion::Animated scale;
        motion::Animated emphasis;
    };

    double target_scale(const Line& line) const;
    void retarget(Line& line, bool snap);
    void paint_line(render::Canvas& canvas, const Line& line, double bottom, color::Argb color) const;

    DisplayStyle style_;
    text::FontStack fonts_;
    text::TextStyle text_style_;
    double secondary_ratio_;
    std::array<Line, 2> lines_;
    bool error_ = false;
    std::string badge_text_;
    text::TextLine badge_;
    motion::Animated badge_opacity_;
    bool shown_ = false;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
};

}
