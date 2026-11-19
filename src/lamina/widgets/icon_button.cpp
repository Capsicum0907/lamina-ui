#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/icon_button.hpp"

#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"

namespace lamina::widgets {

namespace {

using tokens::motion_scheme;

}

IconButton::IconButton(IconButtonOptions options)
    : options_(std::move(options)),
      corner_(0.0, motion_scheme.default_effects()),
      state_layer_(0.0, motion_scheme.fast_effects()),
      checked_(options_.checked ? 1.0 : 0.0, motion_scheme.default_effects()) {
    corner_.snap_to(target_corner());
}

double IconButton::width() const {
    return tokens::icon_button_width(options_.size, options_.width);
}

double IconButton::height() const {
    return tokens::icon_button_size(options_.size).height;
}

render::Rect IconButton::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

double IconButton::target_corner() const {
    const auto& size = tokens::icon_button_size(options_.size);
    const bool round = options_.shape == tokens::ButtonShape::round;
    tokens::CornerSize corner = round ? tokens::CornerSize::full : size.square_corner;
    if (press_.pressed) {
        corner = size.pressed_corner;
    } else if (options_.toggleable && options_.checked) {
        corner = round ? size.selected_round_corner : size.selected_square_corner;
    }
    return tokens::corner_radius(corner, width(), height());
}

void IconButton::retarget() {
    corner_.animate_to(target_corner());
    state_layer_.animate_to(press_.state_layer_opacity());
    checked_.animate_to(options_.checked ? 1.0 : 0.0);
}

void IconButton::set_checked(bool checked) {
    options_.checked = checked;
    retarget();
}

bool IconButton::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, touch_target(bounds()));
    if (clicked && options_.toggleable) {
        options_.checked = !options_.checked;
    }
    retarget();
    return clicked;
}

void IconButton::step(double dt) {
    corner_.step(dt);
    state_layer_.step(dt);
    checked_.step(dt);
}

bool IconButton::animating() const {
    return !corner_.settled() || !state_layer_.settled() || !checked_.settled();
}

void IconButton::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::icon_button_style(options_.style);
    const auto& size = tokens::icon_button_size(options_.size);
    const render::Rect r = bounds();
    const double radius = corner_.value();
    const tokens::IconButtonColors& from = options_.toggleable ? style.unchecked : style.plain;
    const tokens::IconButtonColors& to = options_.toggleable ? style.checked : style.plain;
    const double t = checked_.value();

    const color::Argb content = color::mix(scheme[from.content], scheme[to.content], t);
    const color::Argb container = mix_optional(scheme, from.container, to.container, t);
    const color::Argb outline = mix_optional(scheme, from.outline, to.outline, t);

    const color::Argb fill = with_state_layer(container, content, state_layer_.value());
    if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius)), fill);
    }
    if (color::alpha(outline) != 0) {
        canvas.fill_path(ring_path(r, radius, size.outline_width), outline, render::FillRule::even_odd);
    }
    icons::draw_centered(canvas, options_.icon, r, size.icon_size, content);
}

}
