#include "lamina/widgets/button.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::SpringSpec& shape_spring() {
    static const auto spec = tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
    return spec;
}

const tokens::SpringSpec& state_spring() {
    static const auto spec = tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
    return spec;
}

bool inside(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

render::Path ring(const render::Rect& r, double radius, double width) {
    render::Path p = render::rounded_rect_path(r, render::uniform_radii(radius));
    const render::Rect inner{r.x + width, r.y + width, r.width - 2 * width, r.height - 2 * width};
    p.append(render::rounded_rect_path(inner, render::uniform_radii(std::max(0.0, radius - width))));
    return p;
}

}

Button::Button(ButtonOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts, options_.label,
                               text::text_style(tokens::type_style(tokens::button_size(options_.size).label)))),
      width_(0.0),
      corner_(0.0, shape_spring()),
      state_layer_(0.0, state_spring()),
      elevation_(0.0, shape_spring()) {
    const auto& size = tokens::button_size(options_.size);
    width_ = std::max(tokens::button_min_width, size.leading_space + label_.width + size.trailing_space);
    corner_.snap_to(rest_corner());
    elevation_.snap_to(tokens::elevation_dp[tokens::button_style(options_.style).elevation]);
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
    if (pressed_) {
        return tokens::state_layer_opacity::pressed;
    }
    return hovered_ ? tokens::state_layer_opacity::hover : 0.0;
}

double Button::target_elevation() const {
    const auto& style = tokens::button_style(options_.style);
    const std::size_t level = pressed_ ? style.pressed_elevation : hovered_ ? style.hovered_elevation : style.elevation;
    return tokens::elevation_dp[level];
}

bool Button::handle(const platform::PointerEvent& e) {
    const bool over = e.action != platform::PointerAction::leave && inside(bounds(), e.x_dp, e.y_dp);
    bool clicked = false;
    hovered_ = over;
    switch (e.action) {
    case platform::PointerAction::down:
        pressed_ = over;
        break;
    case platform::PointerAction::up:
        clicked = pressed_ && over;
        pressed_ = false;
        break;
    case platform::PointerAction::leave:
        pressed_ = false;
        break;
    case platform::PointerAction::move:
        break;
    }
    corner_.animate_to(pressed_ ? pressed_corner() : rest_corner());
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
        canvas.fill_path(ring(r, radius, size.outline_width), scheme[*style.outline], render::FillRule::even_odd);
    }

    const double line_box = std::max(label_.style.line_height, label_.ascent + label_.descent);
    const render::Point at{r.x + (r.width - label_.width) / 2.0, r.y + (r.height - line_box) / 2.0};
    text::draw_line(canvas, fonts_, label_, at, label_color);
}

}
