#include "lamina/style/style.hpp"
#include "lamina/widgets/label.hpp"

#include <cmath>

namespace lamina::widgets {

Label::Label(LabelOptions options, const text::FontStack& fonts)
    : text_(std::move(options.text)),
      style_(current_style().type.text(options.type)),
      color_(options.color),
      fonts_(fonts),
      wrap_(options.wrap) {}

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
