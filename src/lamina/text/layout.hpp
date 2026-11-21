#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/style.hpp"

namespace lamina::text {

struct PositionedGlyph {
    std::size_t font;
    std::uint32_t glyph;
    std::uint32_t cluster;
    double x;
    double y;
};

struct TextLine {
    std::vector<PositionedGlyph> glyphs;
    TextStyle style;
    double width;
    double ascent;
    double descent;
    double baseline;
    std::size_t first;
    std::size_t last;

    double box_height() const { return std::max(style.line_height, ascent + descent); }
};

struct Underline {
    double offset;
    double thickness;
};

struct TextBlock {
    std::vector<TextLine> lines;
    double width = 0.0;
    double height = 0.0;
};

inline Instance instance_of(const TextStyle& style) {
    return {style.weight, style.size};
}

TextLine layout_line(const FontStack& fonts, std::string_view utf8, const TextStyle& style);

TextBlock layout_paragraph(const FontStack& fonts, std::string_view utf8, const TextStyle& style, double max_width);

Underline underline(const FontStack& fonts, const TextStyle& style);

double caret_x(const TextLine& line, std::size_t index);
std::size_t index_at(const TextLine& line, double x);

render::Path line_path(const FontStack& fonts, const TextLine& line, render::Point top_left);

void draw_line(render::Canvas& canvas, const FontStack& fonts, const TextLine& line, render::Point top_left,
               color::Argb color);

void draw_block(render::Canvas& canvas, const FontStack& fonts, const TextBlock& block, render::Point top_left,
                color::Argb color);

}
