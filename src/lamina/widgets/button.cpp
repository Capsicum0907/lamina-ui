#include "lamina/widgets/button.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const motion::SpringSpec& shape_spring() {
    static const auto spec = tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
    return spec;
}

const motion::SpringSpec& state_spring() {
    static const auto spec = tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
    return spec;
}

}

Button::Button(ButtonOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts, options_.label,
                               tokens::text_style(tokens::type_style(tokens::button_size(options_.size).label)))),
      width_(0.0),
      corner_(0.0, shape_spring()),
      state_layer_(0.0, state_spring()),
      elevation_(0.0, shape_spring()) {
    const auto& size = tokens::button_size(options_.size);
    width_ = std::max(tokens::button_min_width, size.leading_space + content_width() + size.trailing_space);
    corner_.snap_to(rest_corner());
    elevation_.snap_to(tokens::elevation_dp[tokens::button_style(options_.style).elevation]);
}

double Button::content_width() const {
    if (!options_.leading_icon) {
        return label_.width;
    }
    const auto& size = tokens::button_size(options_.size);
    return size.icon_size + size.icon_label_space + label_.width;
}

double Button::height() const {
    return tokens::button_size(options_.size).height;
}

render::Rect Button::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

double Button::rest_corner() const {
    const auto& size = tokens::button_size(options_.size);
    if (options_.shape == tokens::ButtonShape::round) {
        return height() / 2.0;
    }
    return tokens::corner_radius(size.square_corner, width_, height());
}

double Button::pressed_corner() const {
    return tokens::corner_radius(tokens::button_size(options_.size).pressed_corner, width_, height());
}

double Button::target_state_layer() const {
    return press_.state_layer_opacity();
}

double Button::target_elevation() const {
    const auto& style = tokens::button_style(options_.style);
    const std::size_t level = press_.pressed ? style.pressed_elevation : press_.hovered ? style.hovered_elevation : style.elevation;
    return tokens::elevation_dp[level];
}

bool Button::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, bounds());
    corner_.animate_to(press_.pressed ? pressed_corner() : rest_corner());
    state_layer_.animate_to(target_state_layer());
    elevation_.animate_to(target_elevation());
    return clicked;
}

void Button::step(double dt) {
    corner_.step(dt);
    state_layer_.step(dt);
    elevation_.step(dt);
}

bool Button::animating() const {
    return !corner_.settled() || !state_layer_.settled() || !elevation_.settled();
}

void Button::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::button_style(options_.style);
    const auto& size = tokens::button_size(options_.size);
    const render::Rect r = bounds();
    const double radius = corner_.value();
    const color::Argb label_color = scheme[style.label];
    const render::Path shape = render::rounded_rect_path(r, render::uniform_radii(radius));

    if (style.container) {
        const color::Argb fill = color::overlay(scheme[*style.container], label_color, state_layer_.value());
        paint_surface(canvas, r, radius, fill, elevation_.value(), scheme[color::Role::shadow]);
    } else if (state_layer_.value() > 0.0) {
        canvas.fill_path(shape, color::with_alpha(label_color, state_layer_.value()));
    }
    if (style.outline) {
        canvas.fill_path(ring_path(r, radius, size.outline_width), scheme[*style.outline], render::FillRule::even_odd);
    }

    const double line_box = std::max(label_.style.line_height, label_.ascent + label_.descent);
    double x = r.x + (r.width - content_width()) / 2.0;
    if (options_.leading_icon) {
        const auto& icon = *options_.leading_icon;
        icon.font.draw(canvas, icon.symbol, {x, r.y + (r.height - size.icon_size) / 2.0}, size.icon_size, label_color);
        x += size.icon_size + size.icon_label_space;
    }
    text::draw_line(canvas, fonts_, label_, {x, r.y + (r.height - line_box) / 2.0}, label_color);
}

}
