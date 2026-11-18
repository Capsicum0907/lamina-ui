#include "lamina/widgets/label.hpp"

#include <cmath>

namespace lamina::widgets {

Label::Label(std::string text, tokens::TypeRole type, color::Role color, const text::FontStack& fonts, bool wrap)
    : text_(std::move(text)), style_(text::text_style(tokens::type_style(type))), color_(color), fonts_(fonts), wrap_(wrap) {}

void Label::set_text(std::string text) {
    text_ = std::move(text);
    laid_out_width_ = -1.0;
}

layout::Size Label::measure(const layout::Constraints& c) {
    const double width = wrap_ && std::isfinite(c.max_width) ? c.max_width : layout::unbounded;
    if (width != laid_out_width_) {
        laid_out_width_ = width;
        block_ = text::layout_paragraph(fonts_, text_, style_, width);
    }
    return {block_.width, block_.height};
}

void Label::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    text::draw_block(canvas, fonts_, block_, origin_, scheme[color_]);
}

}
