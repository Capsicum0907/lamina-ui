#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/icon_button.hpp"

#include <algorithm>
#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"

namespace lamina::widgets {

namespace {

using tokens::motion_scheme;

}

IconButton::IconButton(IconButtonOptions options)
    : options_(std::move(options)),
      enabled_(options_.enabled),
      corner_(0.0, motion_scheme.default_effects()),
      state_layer_(0.0, motion_scheme.fast_effects()),
      checked_(options_.checked ? 1.0 : 0.0, motion_scheme.default_effects()),
      icon_fade_(1.0, motion_scheme.fast_effects()) {
    options_.accessible_name = required_name(std::move(options_.accessible_name), "An icon button");
    corner_.snap_to(target_corner());
}

double IconButton::width() const {
    const double removed = tokens::icon_button_size(options_.size).height - height();
    return std::max(tokens::icon_button_size(options_.size).icon_size, tokens::icon_button_width(options_.size, options_.width) - removed);
}

double IconButton::height() const {
    return tokens::icon_button_density(options_.size).at(options_.density);
}

void IconButton::set_density(tokens::Density density) {
    options_.density = density;
    corner_.snap_to(target_corner());
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
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    checked_.animate_to(options_.checked ? 1.0 : 0.0);
}

void IconButton::set_checked(bool checked) {
    options_.checked = checked;
    retarget();
}

void IconButton::set_icon(icons::Icon icon) {
    if (icon.symbol == options_.icon.symbol) {
        return;
    }
    previous_icon_ = std::move(options_.icon);
    options_.icon = std::move(icon);
    icon_fade_.snap_to(0.0);
    icon_fade_.animate_to(1.0);
}

void IconButton::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    retarget();
}

void IconButton::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    retarget();
}

std::optional<TargetHit> IconButton::target_hit(const input::PointerEvent& e) const {
    return widgets::target_hit(bounds(), e.x_dp, e.y_dp, tokens::minimum_target(options_.density));
}

bool IconButton::hits(const input::PointerEvent& e) const {
    return contains(hit_area(e, bounds(), tokens::minimum_target(options_.density)), e.x_dp, e.y_dp);
}

bool IconButton::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (!enabled()) {
        return false;
    }
    const bool clicked = press_.update(e, hit_area(e, bounds(), tokens::minimum_target(options_.density)));
    if (clicked && options_.toggleable) {
        options_.checked = !options_.checked;
    }
    retarget();
    return clicked;
}

bool IconButton::handle(const input::KeyEvent& e) {
    if (!enabled() || !focus_.activates(e, KeyActivation::space_or_enter)) {
        return false;
    }
    if (options_.toggleable) {
        options_.checked = !options_.checked;
    }
    retarget();
    return true;
}

void IconButton::cancel_interaction() {
    press_ = {};
    retarget();
}

void IconButton::step(double dt) {
    corner_.step(dt);
    state_layer_.step(dt);
    checked_.step(dt);
    icon_fade_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
}

bool IconButton::animating() const {
    return !corner_.settled() || !state_layer_.settled() || !checked_.settled() || !icon_fade_.settled() || !focus_.settled() ||
           !enabled_.settled();
}

render::Rect IconButton::icon_rect() const {
    const double s = tokens::icon_button_size(options_.size).icon_size;
    const render::Rect r = bounds();
    return {r.x + (r.width - s) / 2.0, r.y + (r.height - s) / 2.0, s, s};
}

void IconButton::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::icon_button_style(options_.style);
    const auto& size = tokens::icon_button_size(options_.size);
    const render::Rect r = bounds();
    const double radius = corner_.value();
    const tokens::IconButtonColors& from = options_.toggleable ? style.unchecked : style.plain;
    const tokens::IconButtonColors& to = options_.toggleable ? style.checked : style.plain;
    const double t = checked_.value();

    const color::Argb enabled_content =
        options_.content ? scheme[*options_.content] : color::mix(scheme[from.content], scheme[to.content], t);
    const color::Argb enabled_container = mix_optional(scheme, from.container, to.container, t);
    const color::Argb enabled_outline = mix_optional(scheme, from.outline, to.outline, t);
    const double outline_presence = motion::lerp(from.outline ? 1.0 : 0.0, to.outline ? 1.0 : 0.0, std::clamp(t, 0.0, 1.0));

    const color::Argb content = enabled_.mix(resolve(scheme, style.disabled.content), enabled_content);
    const color::Argb outline =
        enabled_.mix(color::scale_alpha(resolve(scheme, style.disabled.outline), outline_presence), enabled_outline);
    const color::Argb fill = enabled_.mix(resolve(scheme, style.disabled.container),
                                          color::state_layer(enabled_container, enabled_content, state_layer_.value()));
    if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius)), fill);
    }
    if (color::alpha(outline) != 0) {
        canvas.fill_path(render::ring_path(r, radius, size.outline_width), outline, render::FillRule::even_odd);
    }
    focus_.paint(canvas, scheme, r, radius);
    const double fade = std::clamp(icon_fade_.value(), 0.0, 1.0);
    if (previous_icon_ && fade < 1.0) {
        icons::draw_centered(canvas, *previous_icon_, r, size.icon_size, color::scale_alpha(content, 1.0 - fade));
    }
    icons::draw_centered(canvas, options_.icon, r, size.icon_size, fade < 1.0 ? color::scale_alpha(content, fade) : content);
    paint_badge(canvas, scheme, badge_, icon_rect());
}

}
