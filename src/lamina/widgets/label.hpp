#pragma once

#include <cstddef>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::widgets {

struct LabelOptions {
    std::string text;
    tokens::TypeRole type = tokens::TypeRole::body_large;
    color::Role color = color::Role::on_surface;
    bool wrap = true;
};

class Label {
public:
    Label(LabelOptions options, const text::FontStack& fonts);

    void set_text(std::string text);
    layout::Size measure(const layout::Constraints& c);
    void set_origin(render::Point origin) { origin_ = origin; }
    void arrange(const render::Rect& r) { origin_ = {r.x, r.y}; }
    render::Rect bounds() const { return {origin_.x, origin_.y, block_.width, block_.height}; }
    std::size_t line_count() const { return block_.lines.size(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    std::string text_;
    text::TextStyle style_;
    color::Role color_;
    text::FontStack fonts_;
    bool wrap_;
    text::TextBlock block_;
    double laid_out_width_ = -1.0;
    render::Point origin_{0.0, 0.0};
};

}
