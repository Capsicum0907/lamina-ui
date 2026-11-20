#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/button.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
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
      enabled_(options_.enabled),
      press_amount_(0.0, motion_scheme.default_effects()),
      squareness_(options_.shape == tokens::ButtonShape::square ? 1.0 : 0.0, motion_scheme.fast_spatial()),
      icon_fade_(1.0, motion_scheme.fast_effects()),
      state_layer_(0.0, motion_scheme.fast_effects()),
      elevation_(0.0, motion_scheme.default_effects()) {
    if (options_.label.empty()) {
        options_.accessible_name = required_name(std::move(options_.accessible_name), "A button without a label");
    }
    width_ = intrinsic_width();
    if (options_.colors) {
        focus_.set_color(options_.colors->focus_ring);
    }
    elevation_.snap_to(target_elevation());
}

double Button::content_width() const {
    if (!options_.leading_icon) {
        return label_.width * label_scale_;
    }
    const auto& size = tokens::button_size(options_.size);
    const double gap = options_.label.empty() ? 0.0 : size.icon_label_space * label_scale_;
    return size.icon_size * icon_scale_ + gap + label_.width * label_scale_;
}

double Button::intrinsic_width() const {
    const auto& size = tokens::button_size(options_.size);
    return std::max(tokens::button_min_width, size.leading_space + content_width() + size.trailing_space);
}

void Button::set_label(std::string label) {
    options_.label = std::move(label);
    label_ = text::layout_line(fonts_, options_.label, tokens::text_style(tokens::type_style(tokens::button_size(options_.size).label)));
    if (!sized_) {
        width_ = intrinsic_width();
    }
}

void Button::set_content_scale(double label_scale, double icon_scale) {
    label_scale_ = label_scale;
    icon_scale_ = icon_scale;
}

void Button::set_shape(tokens::ButtonShape shape) {
    options_.shape = shape;
    squareness_.animate_to(shape == tokens::ButtonShape::square ? 1.0 : 0.0);
}

void Button::set_icon(std::optional<icons::Icon> icon) {
    const bool same = icon && options_.leading_icon && icon->symbol == options_.leading_icon->symbol;
    if (same || (!icon && !options_.leading_icon)) {
        return;
    }
    previous_icon_ = std::move(options_.leading_icon);
    options_.leading_icon = std::move(icon);
    icon_fade_.snap_to(0.0);
    icon_fade_.animate_to(1.0);
    if (!sized_) {
        width_ = intrinsic_width();
    }
}

void Button::set_size(double width, double height) {
    sized_ = true;
    width_ = width;
    height_ = height;
}

render::Rect Button::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

double Button::rest_corner() const {
    const double square = tokens::corner_radius(tokens::button_size(options_.size).square_corner, width_, height());
    return motion::lerp(height() / 2.0, square, squareness_.value());
}

double Button::pressed_corner() const {
    return tokens::corner_radius(tokens::button_size(options_.size).pressed_corner, width_, height());
}

double Button::corner() const {
    const double radius = motion::lerp(rest_corner(), pressed_corner(), press_amount_.value());
    return std::clamp(radius, 0.0, std::min(width_, height()) / 2.0);
}

double Button::target_state_layer() const {
    return focus_.state_layer_opacity(press_.state_layer_opacity());
}

double Button::target_elevation() const {
    const auto& style = tokens::button_style(options_.style);
    return enabled() ? state_elevation(style, press_) : tokens::elevation_dp[style.disabled_elevation];
}

void Button::retarget() {
    press_amount_.animate_to(press_.pressed ? 1.0 : 0.0);
    state_layer_.animate_to(target_state_layer());
    elevation_.animate_to(target_elevation());
}

void Button::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    retarget();
}

void Button::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    retarget();
}

bool Button::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (!enabled()) {
        return false;
    }
    const bool clicked = press_.update(e, bounds());
    retarget();
    return clicked;
}

bool Button::handle(const input::KeyEvent& e) {
    return enabled() && focus_.activates(e, KeyActivation::space_or_enter);
}

void Button::cancel_interaction() {
    press_ = {};
    retarget();
}

void Button::step(double dt) {
    press_amount_.step(dt);
    squareness_.step(dt);
    icon_fade_.step(dt);
    state_layer_.step(dt);
    elevation_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
}

bool Button::animating() const {
    return !press_amount_.settled() || !squareness_.settled() || !icon_fade_.settled() || !state_layer_.settled() ||
           !elevation_.settled() || !focus_.settled() || !enabled_.settled();
}

void Button::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::button_style(options_.style);
    const auto& size = tokens::button_size(options_.size);
    const render::Rect r = bounds();
    const double radius = corner();
    const std::optional<color::Role> container = options_.colors ? options_.colors->container : style.container;
    const color::Argb enabled_label = scheme[options_.colors ? options_.colors->content : style.label];
    const color::Argb label_color = enabled_.mix(resolve(scheme, style.disabled_label), enabled_label);
    const render::Path shape = render::rounded_rect_path(r, render::uniform_radii(radius));

    const color::Argb enabled_fill = color::state_layer(container ? scheme[*container] : 0, enabled_label, state_layer_.value());
    const color::Argb fill = enabled_.mix(resolve(scheme, style.disabled_container), enabled_fill);
    if (container) {
        paint_surface(canvas, r, radius, fill, elevation_.value(), scheme[color::Role::shadow]);
    } else if (color::alpha(fill) != 0) {
        canvas.fill_path(shape, fill);
    }
    if (style.outline) {
        const color::Argb outline = enabled_.mix(resolve(scheme, style.disabled_outline), scheme[*style.outline]);
        canvas.fill_path(render::ring_path(r, radius, size.outline_width), outline, render::FillRule::even_odd);
    }

    focus_.paint(canvas, scheme, r, radius);

    double x = r.x + (r.width - content_width()) / 2.0;
    if (options_.leading_icon) {
        const double icon_size = size.icon_size * icon_scale_;
        const double y = r.y + (r.height - icon_size) / 2.0;
        const double fade = std::clamp(icon_fade_.value(), 0.0, 1.0);
        if (previous_icon_ && fade < 1.0) {
            draw_icon(canvas, *previous_icon_, x, y, icon_size, label_color, 1.0 - fade);
        }
        draw_icon(canvas, *options_.leading_icon, x, y, icon_size, label_color, fade);
        x += icon_size + size.icon_label_space * label_scale_;
    }
    if (label_scale_ == 1.0) {
        text::draw_line(canvas, fonts_, label_, {x, r.y + (r.height - label_.box_height()) / 2.0}, label_color);
        return;
    }
    canvas.save();
    canvas.translate(x, r.y + (r.height - label_.box_height() * label_scale_) / 2.0);
    canvas.scale(label_scale_);
    text::draw_line(canvas, fonts_, label_, {0.0, 0.0}, label_color);
    canvas.restore();
}

void Button::draw_icon(render::Canvas& canvas, const icons::Icon& icon, double x, double y, double size, color::Argb color,
                       double opacity) const {
    if (opacity <= 0.0) {
        return;
    }
    icon.font.draw(canvas, icon.symbol, {x, y}, size, opacity < 1.0 ? color::scale_alpha(color, opacity) : color,
                   options_.icon_style);
}

}
