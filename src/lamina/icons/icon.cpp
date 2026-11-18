#include "lamina/icons/icon.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace lamina::icons {

IconFont::IconFont(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    if (!font_) {
        throw std::invalid_argument("icon font is null");
    }
    for (Symbol s : all_symbols) {
        if (!has(s)) {
            throw std::invalid_argument("icon font lacks a symbol of the lamina symbol table");
        }
    }
}

IconFont IconFont::from_file(const std::string& path) {
    return IconFont(text::Font::from_file(path));
}

IconFont IconFont::from_directory(const std::string& directory) {
    return from_file(directory + "/" + symbols_font_file);
}

bool IconFont::has(Symbol symbol) const {
    return font_->glyph_for(static_cast<char32_t>(symbol)) != 0;
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
    const render::Path& em = font_->glyph_outline_em(font_->glyph_for(static_cast<char32_t>(symbol)), axes);
    return em.transformed({size, 0.0, 0.0, size, top_left.x, top_left.y + size});
}

void IconFont::draw(render::Canvas& canvas, Symbol symbol, render::Point top_left, double size, color::Argb color,
                    const IconStyle& style) const {
    canvas.fill_path(path(symbol, top_left, size, style), color);
}

}
