#include "lamina/text/layout.hpp"

#include <algorithm>

namespace lamina::text {

TextLine layout_line(const FontStack& fonts, std::string_view utf8, const TextStyle& style) {
    TextLine line{{}, style, 0.0, 0.0, 0.0, 0.0};
    if (fonts.empty()) {
        return line;
    }
    const std::u32string text = decode_utf8(utf8);

    const FontExtents primary = fonts.fonts().front()->extents_em();
    line.ascent = primary.ascender * style.size;
    line.descent = -primary.descender * style.size;

    double x = 0.0;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t font_index = fonts.font_for(text[start]);
        std::size_t end = start + 1;
        while (end < text.size() && fonts.font_for(text[end]) == font_index) {
            ++end;
        }
        const Font& font = *fonts.fonts()[font_index];
        const FontExtents e = font.extents_em();
        line.ascent = std::max(line.ascent, e.ascender * style.size);
        line.descent = std::max(line.descent, -e.descender * style.size);

        for (const ShapedGlyph& g : font.shape(std::u32string_view(text).substr(start, end - start), start, style.weight)) {
            line.glyphs.push_back({font_index, g.glyph, g.cluster, x + g.offset_x_em * style.size, -g.offset_y_em * style.size});
            x += g.advance_em * style.size + style.tracking;
        }
        start = end;
    }
    if (!line.glyphs.empty()) {
        x -= style.tracking;
    }
    line.width = std::max(0.0, x);
    const double box = std::max(style.line_height, line.ascent + line.descent);
    line.baseline = (box - (line.ascent + line.descent)) / 2.0 + line.ascent;
    return line;
}

render::Path line_path(const FontStack& fonts, const TextLine& line, render::Point top_left) {
    render::Path out;
    const double s = line.style.size;
    for (const PositionedGlyph& g : line.glyphs) {
        const render::Path& outline = fonts.fonts()[g.font]->glyph_outline_em(g.glyph, line.style.weight);
        const render::Affine m{s, 0.0, 0.0, s, top_left.x + g.x, top_left.y + line.baseline + g.y};
        out.append(outline.transformed(m));
    }
    return out;
}

void draw_line(render::Canvas& canvas, const FontStack& fonts, const TextLine& line, render::Point top_left,
               color::Argb color) {
    const render::Path path = line_path(fonts, line, top_left);
    if (!path.empty()) {
        canvas.fill_path(path, color);
    }
}

}
