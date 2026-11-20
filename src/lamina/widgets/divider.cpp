#include "lamina/widgets/divider.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::widgets {

namespace {

constexpr const tokens::DividerTokens& t = tokens::divider_tokens;

bool insets_start(DividerInset inset) {
    return inset == DividerInset::start || inset == DividerInset::both;
}

bool insets_end(DividerInset inset) {
    return inset == DividerInset::end || inset == DividerInset::both;
}

}

layout::Size Divider::measure(const layout::Constraints& c) const {
    if (options_.orientation == DividerOrientation::horizontal) {
        return c.constrain({std::isfinite(c.max_width) ? c.max_width : c.min_width, options_.thickness});
    }
    return c.constrain({options_.thickness, std::isfinite(c.max_height) ? c.max_height : c.min_height});
}

render::Rect Divider::line() const {
    const double start = insets_start(options_.inset) ? t.inset_start : 0.0;
    const double end = insets_end(options_.inset) ? t.inset_end : 0.0;
    if (options_.orientation == DividerOrientation::horizontal) {
        const double width = std::max(0.0, rect_.width - start - end);
        return {rect_.x + start, rect_.y + (rect_.height - options_.thickness) / 2.0, width, options_.thickness};
    }
    const double height = std::max(0.0, rect_.height - start - end);
    return {rect_.x + (rect_.width - options_.thickness) / 2.0, rect_.y + start, options_.thickness, height};
}

void Divider::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = line();
    if (r.width <= 0.0 || r.height <= 0.0) {
        return;
    }
    canvas.fill_rect(r, scheme[options_.color]);
}

}
