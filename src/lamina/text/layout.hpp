#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::text {

struct TextStyle {
    double size;
    double line_height;
    int weight;
    double tracking;
};

constexpr TextStyle text_style(const tokens::TypeStyle& t) {
    return {t.size_sp, t.line_height_sp, t.weight, t.tracking_sp};
}

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
};

TextLine layout_line(const FontStack& fonts, std::string_view utf8, const TextStyle& style);

render::Path line_path(const FontStack& fonts, const TextLine& line, render::Point top_left);

void draw_line(render::Canvas& canvas, const FontStack& fonts, const TextLine& line, render::Point top_left,
               color::Argb color);

}
