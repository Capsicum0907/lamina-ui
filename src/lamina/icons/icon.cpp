#include "lamina/icons/icon.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "lamina/builtin_resources.hpp"
#include "lamina/tokens/raster.hpp"

namespace lamina::icons {

IconFont::IconFont(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    if (!font_) {
        throw std::invalid_argument("icon font is null");
    }
    for (Symbol s : builtin::all_icons) {
        if (!has(s)) {
            throw std::invalid_argument("icon font lacks a symbol that lamina draws itself; cut it with lamina_add_resources");
        }
    }
}

IconFont IconFont::from_resource(const resources::Resource& resource) {
    return IconFont(text::Font::from_resource(resource));
}

bool IconFont::has(Symbol symbol) const {
    return font_->glyph_for(symbol.code) != 0;
}

render::Path IconFont::path(Symbol symbol, render::Point top_left, double size, const IconStyle& style) const {
    const double opsz = std::clamp(style.optical_size.value_or(size), tokens::symbol_axes.optical_size_min,
                                   tokens::symbol_axes.optical_size_max);
    const std::array<text::Variation, 4> axes{{
        {text::axis_tag('F', 'I', 'L', 'L'), static_cast<float>(style.fill)},
        {text::axis_tag('w', 'g', 'h', 't'), static_cast<float>(style.weight)},
        {text::axis_tag('G', 'R', 'A', 'D'), static_cast<float>(style.grade)},
        {text::axis_tag('o', 'p', 's', 'z'), static_cast<float>(opsz)},
    }};
    const render::Path& em = font_->glyph_outline_em(font_->glyph_for(symbol.code), axes);
    return em.transformed({size, 0.0, 0.0, size, top_left.x, top_left.y + size});
}

void IconFont::draw(render::Canvas& canvas, Symbol symbol, render::Point top_left, double size, color::Argb color,
                    const IconStyle& style) const {
    if (canvas.quick_reject({top_left.x, top_left.y, size, size}, size * tokens::raster.cull_glyph_outset_em)) {
        canvas.count_rejected(0);
        return;
    }
    canvas.fill_path(path(symbol, top_left, size, style), color);
}

void draw_centered(render::Canvas& canvas, const Icon& icon, const render::Rect& rect, double size, color::Argb color,
                   const IconStyle& style) {
    icon.font.draw(canvas, icon.symbol, {rect.x + (rect.width - size) / 2.0, rect.y + (rect.height - size) / 2.0}, size, color,
                   style);
}

void draw_centered(render::Canvas& canvas, const Icon& icon, render::Point center, double size, color::Argb color,
                   const IconStyle& style) {
    icon.font.draw(canvas, icon.symbol, {center.x - size / 2.0, center.y - size / 2.0}, size, color, style);
}

}
