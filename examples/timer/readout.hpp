#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::timer {

class DigitFace {
public:
    DigitFace(const text::FontStack& fonts, tokens::TypeRole role);

    double width(std::string_view text) const;
    double height() const { return height_; }
    double baseline() const { return baseline_; }
    void draw(render::Canvas& canvas, std::string_view text, render::Point top_left,
              const std::function<color::Argb(std::size_t)>& color_of) const;

private:
    struct Glyph {
        std::string text;
        text::TextLine line;
    };

    const Glyph& glyph(std::string_view character) const;
    double advance(const Glyph& g) const;

    text::FontStack fonts_;
    text::TextStyle style_;
    mutable std::vector<Glyph> glyphs_;
    double digit_width_ = 0.0;
    double height_ = 0.0;
    double baseline_ = 0.0;
};

std::vector<std::string_view> split_characters(std::string_view utf8);

struct EntryReadoutStyle {
    tokens::TypeRole digit_type = tokens::TypeRole::display_large;
    tokens::TypeRole unit_type = tokens::TypeRole::title_large;
    color::Role entered = color::Role::on_surface;
    color::Role pending = color::Role::outline_variant;
    color::Role unit = color::Role::on_surface_variant;
    double unit_gap = 4.0;
    double field_gap = 20.0;
    double max_scale = 1.6;
    std::array<const char*, 3> units{"h", "m", "s"};
    motion::SpringSpec fade_spring = current_style().motion.default_effects();
};

class EntryReadout {
public:
    EntryReadout(EntryReadoutStyle style, const text::FontStack& fonts);

    void show(const std::string& digits, std::size_t entered);
    void arrange(const render::Rect& rect) { rect_ = rect; }
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    EntryReadoutStyle style_;
    text::FontStack fonts_;
    DigitFace digits_;
    DigitFace units_;
    std::string shown_;
    std::vector<motion::Animated> emphasis_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
};

struct TimeTextStyle {
    tokens::TypeRole type = tokens::TypeRole::display_large;
    motion::SpringSpec scale_spring = current_style().motion.default_spatial();
};

class TimeText {
public:
    TimeText(TimeTextStyle style, const text::FontStack& fonts);

    void show(const std::string& text);
    void arrange(const render::Rect& rect);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, color::Argb color) const;

private:
    double fit() const;

    TimeTextStyle style_;
    DigitFace face_;
    std::string text_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    motion::Animated scale_;
    bool placed_ = false;
};

}
