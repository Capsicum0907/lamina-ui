#include "lamina/style/style.hpp"
#include "lamina/widgets/badge.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"

namespace lamina::widgets {

namespace {

const tokens::BadgeTokens& t() {
    return current_style().badge;
}

text::TextStyle label_style() {
    return current_style().type.text(t().label);
}

}

std::string badge_count(long long count, long long max) {
    if (count > max) {
        return std::to_string(max) + t().overflow_suffix;
    }
    return std::to_string(std::max(0LL, count));
}

render::Rect badge_rect(const render::Rect& anchor, layout::Size badge, bool has_content, const std::optional<render::Rect>& bounds) {
    const double h_offset = has_content ? t().large_horizontal_offset : t().small_offset;
    const double v_offset = has_content ? t().large_vertical_offset : t().small_offset;
    double x = anchor.x + anchor.width - h_offset;
    double y = anchor.y - badge.height + v_offset;
    if (bounds) {
        x = std::min(x, bounds->x + bounds->width - badge.width);
        y = std::max(y, bounds->y);
    }
    return {x, y, badge.width, badge.height};
}

Badge::Badge(BadgeSpec spec, const text::FontStack& fonts) : spec_(std::move(spec)), fonts_(fonts), line_(text::layout_line(fonts_, spec_.text, label_style())) {}

void Badge::restyle() {
    line_ = text::layout_line(fonts_, spec_.text, label_style());
}

void Badge::set_spec(BadgeSpec spec) {
    spec_ = std::move(spec);
    line_ = text::layout_line(fonts_, spec_.text, label_style());
}

layout::Size Badge::size() const {
    if (small()) {
        return {t().small_size, t().small_size};
    }
    return {std::max(t().large_size, line_.width + 2.0 * t().large_horizontal_padding), t().large_size};
}

render::Rect Badge::place(const render::Rect& anchor, const std::optional<render::Rect>& bounds) const {
    const layout::Size s = size();
    return badge_rect(anchor, s, s.width > t().small_size, bounds);
}

void Badge::paint(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& anchor,
                  const std::optional<render::Rect>& bounds, double opacity) const {
    if (opacity <= 0.0) {
        return;
    }
    const render::Rect r = place(anchor, bounds);
    canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(std::min(r.width, r.height) / 2.0)),
                     color::scale_alpha(scheme[t().container], opacity));
    if (small()) {
        return;
    }
    const double x = r.x + (r.width - line_.width) / 2.0;
    const double y = r.y + (r.height - line_.box_height()) / 2.0;
    text::draw_line(canvas, fonts_, line_, {x, y}, color::scale_alpha(scheme[t().content], opacity));
}

void paint_badge(render::Canvas& canvas, const color::ColorScheme& scheme, const std::optional<Badge>& badge, const render::Rect& anchor,
                 const std::optional<render::Rect>& bounds) {
    if (badge) {
        badge->paint(canvas, scheme, anchor, bounds);
    }
}

}
