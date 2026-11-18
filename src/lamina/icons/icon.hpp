#pragma once

#include <memory>
#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbols.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/icon.hpp"

namespace lamina::icons {

struct IconStyle {
    double fill = tokens::symbol_axes.fill;
    double weight = tokens::symbol_axes.weight;
    double grade = tokens::symbol_axes.grade;
    std::optional<double> optical_size = std::nullopt;
};

class IconFont {
public:
    explicit IconFont(std::shared_ptr<text::Font> font);
    static IconFont from_file(const std::string& path);
    static IconFont from_directory(const std::string& directory);

    const text::Font& font() const { return *font_; }
    bool has(Symbol symbol) const;

    render::Path path(Symbol symbol, render::Point top_left, double size, const IconStyle& style = {}) const;
    void draw(render::Canvas& canvas, Symbol symbol, render::Point top_left, double size, color::Argb color,
              const IconStyle& style = {}) const;

private:
    std::shared_ptr<text::Font> font_;
};

struct Icon {
    IconFont font;
    Symbol symbol;
};

}
