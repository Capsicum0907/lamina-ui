#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/button.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

using tokens::motion_scheme;

}

Button::Button(ButtonOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts, options_.label,
                               tokens::text_style(tokens::type_style(tokens::button_size(options_.size).label)))),
      width_(0.0),
      height_(tokens::button_size(options_.size).height),
      corner_(0.0, motion_scheme.default_effects()),
      state_layer_(0.0, motion_scheme.fast_effects()),
      elevation_(0.0, motion_scheme.default_effects()) {
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
    const double gap = options_.label.empty() ? 0.0 : size.icon_label_space;
    return size.icon_size + gap + label_.width;
}

void Button::set_size(double width, double height) {
    if (width == width_ && height == height_) {
        return;
    }
    width_ = width;
    height_ = height;
    const double corner = press_.pressed ? pressed_corner() : rest_corner();
    if (corner_.settled()) {
        corner_.snap_to(corner);
    } else {
        corner_.animate_to(corner);
    }
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

void Button::retarget() {
    corner_.animate_to(press_.pressed ? pressed_corner() : rest_corner());
    state_layer_.animate_to(target_state_layer());
    elevation_.animate_to(state_elevation(tokens::button_style(options_.style), press_));
}

bool Button::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, bounds());
    retarget();
    return clicked;
}

void Button::cancel_interaction() {
    press_ = {};
    retarget();
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

    const color::Argb fill = color::state_layer(style.container ? scheme[*style.container] : 0, label_color, state_layer_.value());
    if (style.container) {
        paint_surface(canvas, r, radius, fill, elevation_.value(), scheme[color::Role::shadow]);
    } else if (color::alpha(fill) != 0) {
        canvas.fill_path(shape, fill);
    }
    if (style.outline) {
        canvas.fill_path(render::ring_path(r, radius, size.outline_width), scheme[*style.outline], render::FillRule::even_odd);
    }

    double x = r.x + (r.width - content_width()) / 2.0;
    if (options_.leading_icon) {
        const auto& icon = *options_.leading_icon;
        icon.font.draw(canvas, icon.symbol, {x, r.y + (r.height - size.icon_size) / 2.0}, size.icon_size, label_color);
        x += size.icon_size + size.icon_label_space;
    }
    text::draw_line(canvas, fonts_, label_, {x, r.y + (r.height - label_.box_height()) / 2.0}, label_color);
}

}
