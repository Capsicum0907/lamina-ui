#pragma once

#include <memory>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/resources/resource.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/icon.hpp"

namespace lamina::icons {

struct IconStyle {
    double fill = tokens::symbol_axes.fill;
    double weight = tokens::symbol_axes.weight;
    double grade = tokens::symbol_axes.grade;
    std::optional<double> optical_size = std::nullopt;
};

constexpr IconStyle mix(const IconStyle& a, const IconStyle& b, double k) {
    return {motion::lerp(a.fill, b.fill, k), motion::lerp(a.weight, b.weight, k), motion::lerp(a.grade, b.grade, k),
            k < 0.5 ? a.optical_size : b.optical_size};
}

class IconFont {
public:
    explicit IconFont(std::shared_ptr<text::Font> font);
    static IconFont from_resource(const resources::Resource& resource);

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

void draw_centered(render::Canvas& canvas, const Icon& icon, const render::Rect& rect, double size, color::Argb color,
                   const IconStyle& style = {});
void draw_centered(render::Canvas& canvas, const Icon& icon, render::Point center, double size, color::Argb color,
                   const IconStyle& style = {});

}
