#include "lamina/text/layout.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/text/line_break.hpp"

namespace lamina::text {

namespace {

constexpr char32_t tab = U'	';

struct TabStops {
    std::size_t font = 0;
    std::uint32_t glyph = 0;
    double interval = 0.0;

    double next(double x) const { return interval > 0.0 ? (std::floor(x / interval) + 1.0) * interval : x; }
};

TabStops tab_stops(const FontStack& fonts, const TextStyle& style) {
    TabStops stops;
    if (fonts.empty()) {
        return stops;
    }
    stops.font = fonts.font_for(U' ');
    const std::vector<ShapedGlyph> space = fonts.fonts()[stops.font]->shape(U" ", 0, instance_of(style));
    if (!space.empty()) {
        stops.glyph = space.front().glyph;
        stops.interval = (space.front().advance_em * style.size + style.tracking) * style.tab_width;
    }
    return stops;
}

TextLine shape_range(const FontStack& fonts, std::u32string_view text, std::size_t first, std::size_t last,
                     const TextStyle& style) {
    TextLine line{{}, style, 0.0, 0.0, 0.0, 0.0, first, last};
    if (fonts.empty()) {
        return line;
    }
    const FontExtents primary = fonts.fonts().front()->extents_em();
    line.ascent = primary.ascender * style.size;
    line.descent = -primary.descender * style.size;

    const TabStops stops = text.substr(first, last - first).find(tab) == std::u32string_view::npos ? TabStops{} : tab_stops(fonts, style);
    double x = 0.0;
    bool tracked = false;
    std::size_t start = first;
    while (start < last) {
        if (text[start] == tab) {
            line.glyphs.push_back({stops.font, stops.glyph, static_cast<std::uint32_t>(start), x, 0.0});
            x = stops.next(x);
            tracked = false;
            ++start;
            continue;
        }
        const std::size_t font_index = fonts.font_for(text[start]);
        std::size_t end = start + 1;
        while (end < last && text[end] != tab && fonts.font_for(text[end]) == font_index) {
            ++end;
        }
        const Font& font = *fonts.fonts()[font_index];
        const FontExtents e = font.extents_em();
        line.ascent = std::max(line.ascent, e.ascender * style.size);
        line.descent = std::max(line.descent, -e.descender * style.size);

        for (const ShapedGlyph& g : font.shape(text.substr(start, end - start), start, instance_of(style))) {
            line.glyphs.push_back({font_index, g.glyph, g.cluster, x + g.offset_x_em * style.size, -g.offset_y_em * style.size});
            x += g.advance_em * style.size + style.tracking;
            tracked = true;
        }
        start = end;
    }
    if (tracked) {
        x -= style.tracking;
    }
    line.width = std::max(0.0, x);
    line.baseline = (line.box_height() - (line.ascent + line.descent)) / 2.0 + line.ascent;
    return line;
}

std::vector<double> codepoint_advances(const TextLine& shaped, std::size_t count, const TextStyle& style) {
    std::vector<double> advances(count, 0.0);
    for (std::size_t i = 0; i < shaped.glyphs.size(); ++i) {
        const double next_x = i + 1 < shaped.glyphs.size() ? shaped.glyphs[i + 1].x : shaped.width + style.tracking;
        const std::size_t c = shaped.glyphs[i].cluster;
        if (c < count) {
            advances[c] += next_x - shaped.glyphs[i].x;
        }
    }
    return advances;
}

}

TextLine layout_line(const FontStack& fonts, std::string_view utf8, const TextStyle& style) {
    const std::u32string text = decode_utf8(utf8);
    return shape_range(fonts, text, 0, text.size(), style);
}

TextLine ellipsize_line(const FontStack& fonts, std::string_view utf8, const TextStyle& style, double max_width,
                        std::string_view ellipsis) {
    const std::u32string text = decode_utf8(utf8);
    TextLine whole = shape_range(fonts, text, 0, text.size(), style);
    if (whole.width <= max_width) {
        return whole;
    }
    const std::u32string mark = decode_utf8(ellipsis);
    std::vector<std::size_t> cuts{0};
    for (const PositionedGlyph& g : whole.glyphs) {
        if (g.cluster > 0 && g.cluster < text.size()) {
            cuts.push_back(g.cluster);
        }
    }
    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
    auto cut_at = [&](std::size_t k) {
        while (k > 0 && is_space(text[k - 1])) {
            --k;
        }
        const std::u32string s = text.substr(0, k) + mark;
        return shape_range(fonts, s, 0, s.size(), style);
    };
    std::size_t lo = 0;
    std::size_t hi = cuts.size();
    while (hi - lo > 1) {
        const std::size_t mid = (lo + hi) / 2;
        if (cut_at(cuts[mid]).width <= max_width) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return cut_at(cuts[lo]);
}

TextBlock layout_paragraph(const FontStack& fonts, std::string_view utf8, const TextStyle& style, double max_width) {
    TextBlock block;
    const std::u32string text = decode_utf8(utf8);
    const TextLine whole = shape_range(fonts, text, 0, text.size(), style);
    const std::vector<double> advance = codepoint_advances(whole, text.size(), style);
    const TabStops stops = text.find(tab) == std::u32string::npos ? TabStops{} : tab_stops(fonts, style);

    auto emit = [&](std::size_t first, std::size_t last) {
        while (last > first && is_space(text[last - 1])) {
            --last;
        }
        block.lines.push_back(shape_range(fonts, text, first, last, style));
    };

    std::size_t line_start = 0;
    std::size_t last_break = 0;
    double width = 0.0;
    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] == U'\n') {
            emit(line_start, i);
            line_start = i + 1;
            last_break = line_start;
            width = 0.0;
            i = line_start;
            continue;
        }
        if (i > line_start && can_break_before(text, i)) {
            last_break = i;
        }
        const double next = text[i] == tab ? stops.next(width) : width + advance[i];
        if (next > max_width && i > line_start && !is_space(text[i])) {
            const std::size_t cut = last_break > line_start ? last_break : i;
            emit(line_start, cut);
            line_start = cut;
            while (line_start < text.size() && is_space(text[line_start])) {
                ++line_start;
            }
            last_break = line_start;
            width = 0.0;
            i = line_start;
            continue;
        }
        width = next;
        ++i;
    }
    if (line_start < text.size() || block.lines.empty() || (!text.empty() && text.back() == U'\n')) {
        emit(line_start, text.size());
    }

    for (const TextLine& line : block.lines) {
        block.width = std::max(block.width, line.width);
        block.height += line.box_height();
    }
    return block;
}

Underline underline(const FontStack& fonts, const TextStyle& style) {
    if (fonts.empty()) {
        return {0.0, 0.0};
    }
    const UnderlineMetrics m = fonts.fonts().front()->underline_em();
    return {-m.position * style.size, m.thickness * style.size};
}

double caret_x(const TextLine& line, std::size_t index) {
    double best = line.width;
    std::size_t best_cluster = line.last;
    for (const PositionedGlyph& g : line.glyphs) {
        if (g.cluster >= index && g.cluster < best_cluster) {
            best_cluster = g.cluster;
            best = g.x;
        }
    }
    return index <= line.first ? 0.0 : best;
}

std::size_t index_at(const TextLine& line, double x) {
    std::size_t best = line.first;
    double best_distance = std::abs(x);
    for (std::size_t i = line.first + 1; i <= line.last; ++i) {
        const double d = std::abs(x - caret_x(line, i));
        if (d < best_distance) {
            best_distance = d;
            best = i;
        }
    }
    return best;
}

render::Path line_path(const FontStack& fonts, const TextLine& line, render::Point top_left) {
    render::Path out;
    const double s = line.style.size;
    for (const PositionedGlyph& g : line.glyphs) {
        const render::Path& outline = fonts.fonts()[g.font]->glyph_outline_em(g.glyph, instance_of(line.style));
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

void draw_block(render::Canvas& canvas, const FontStack& fonts, const TextBlock& block, render::Point top_left,
                color::Argb color) {
    double y = top_left.y;
    for (const TextLine& line : block.lines) {
        draw_line(canvas, fonts, line, {top_left.x, y}, color);
        y += line.box_height();
    }
}

}
