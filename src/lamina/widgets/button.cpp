#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/button.hpp"

#include <algorithm>
#include <stdexcept>

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

ButtonShapes toggle_button_shapes(tokens::ButtonSize size) {
    return {tokens::toggle_button_rest_corners(size), tokens::toggle_button_pressed_corners(size), tokens::toggle_button_checked_corners(size),
            tokens::toggle_button_motion.shape};
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
      elevation_(0.0, motion_scheme.default_effects()),
      checked_amount_(options_.toggle && options_.toggle->checked ? 1.0 : 0.0, tokens::toggle_button_motion.colors),
      border_(1.0, tokens::toggle_button_motion.border_width) {
    if (options_.toggle && options_.toggle->checked_colors && !tokens::toggle_button_style(options_.style)) {
        throw std::invalid_argument("A text button cannot be a toggle button");
    }
    if (options_.toggle && !options_.shapes) {
        options_.shapes = toggle_button_shapes(options_.size);
    }
    if (options_.shapes) {
        corners_.emplace(target_corners(), options_.shapes->spring);
    }
    border_.snap_to(checked() && options_.toggle->checked_colors ? 0.0 : 1.0);
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
    return icon_size() * icon_scale_ + gap + label_.width * label_scale_;
}

double Button::intrinsic_width() const {
    const ButtonPadding pad = padding();
    return std::max(options_.min_width.value_or(tokens::button_min_width), pad.leading + content_width() + pad.trailing);
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

tokens::Corners Button::target_corners() const {
    const ButtonShapes& s = *options_.shapes;
    if (press_.pressed) {
        return s.pressed;
    }
    if (checked() && s.checked) {
        return *s.checked;
    }
    return s.shape;
}

void Button::set_shapes(ButtonShapes shapes) {
    options_.shapes = shapes;
    if (!corners_) {
        corners_.emplace(target_corners(), shapes.spring);
        return;
    }
    corners_->animate_to(target_corners());
}

ButtonPadding Button::padding() const {
    const auto& size = tokens::button_size(options_.size);
    return options_.padding.value_or(ButtonPadding{size.leading_space, size.trailing_space});
}

double Button::icon_size() const {
    return options_.icon_size.value_or(tokens::button_size(options_.size).icon_size);
}

render::CornerRadii Button::corner_radii() const {
    if (corners_) {
        return corners_->radii(width_, height());
    }
    return render::uniform_radii(corner());
}

double Button::content_offset() const {
    if (!options_.center_optically || !corners_) {
        return 0.0;
    }
    const ButtonPadding pad = padding();
    return std::clamp(corners_->optical_offset(width_, height()), -pad.leading, pad.trailing);
}

AccessibleNode Button::accessible() const {
    AccessibleState state{.enabled = enabled(), .focusable = enabled(), .focused = focused()};
    AccessibleRole role = AccessibleRole::button;
    if (options_.toggle) {
        if (options_.toggle->radio) {
            role = AccessibleRole::radio_button;
            state.selected = checked();
        } else {
            role = AccessibleRole::toggle_button;
            state.toggle = checked() ? AccessibleToggle::on : AccessibleToggle::off;
        }
    }
    return {role, accessible_name(), bounds(), state};
}

void Button::set_checked(bool checked) {
    if (!options_.toggle || options_.toggle->checked == checked) {
        return;
    }
    options_.toggle->checked = checked;
    checked_amount_.animate_to(checked ? 1.0 : 0.0);
    border_.animate_to(checked && options_.toggle->checked_colors ? 0.0 : 1.0);
    retarget();
}

void Button::activate() {
    if (options_.toggle) {
        set_checked(options_.toggle->radio ? true : !options_.toggle->checked);
    }
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
    if (corners_) {
        corners_->animate_to(target_corners());
    }
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
    if (clicked) {
        activate();
    }
    retarget();
    return clicked;
}

bool Button::handle(const input::KeyEvent& e) {
    if (!enabled() || !focus_.activates(e, KeyActivation::space_or_enter)) {
        return false;
    }
    activate();
    return true;
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
    if (corners_) {
        corners_->step(dt);
    }
    checked_amount_.step(dt);
    border_.step(dt);
}

bool Button::animating() const {
    return !press_amount_.settled() || !squareness_.settled() || !icon_fade_.settled() || !state_layer_.settled() ||
           !elevation_.settled() || !focus_.settled() || !enabled_.settled() || (corners_ && !corners_->settled()) ||
           !checked_amount_.settled() || !border_.settled();
}

void Button::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::button_style(options_.style);
    const auto& size = tokens::button_size(options_.size);
    const render::Rect r = bounds();
    const render::CornerRadii radii = corner_radii();
    const double k = std::clamp(checked_amount_.value(), 0.0, 1.0);
    const bool toggle_colors = options_.toggle && options_.toggle->checked_colors && tokens::toggle_button_style(options_.style);

    std::optional<color::Role> container = options_.colors ? options_.colors->container : style.container;
    color::Argb enabled_label = scheme[options_.colors ? options_.colors->content : style.label];
    color::Argb enabled_container = container ? scheme[*container] : 0;
    color::Argb disabled_label = resolve(scheme, style.disabled_label);
    color::Argb disabled_container = resolve(scheme, style.disabled_container);
    if (toggle_colors) {
        const tokens::ToggleButtonColorTokens& t = *tokens::toggle_button_style(options_.style);
        const color::Argb rest = t.container ? scheme[*t.container] : color::with_alpha(scheme[t.checked_container], 0.0);
        enabled_container = color::mix(rest, scheme[t.checked_container], k);
        enabled_label = color::mix(scheme[t.content], scheme[t.checked_content], k);
        disabled_label = resolve(scheme, t.disabled_content);
        disabled_container = resolve(scheme, t.disabled_container);
        container = t.checked_container;
    }
    const color::Argb label_color = enabled_.mix(disabled_label, enabled_label);

    const color::Argb enabled_fill = color::state_layer(enabled_container, enabled_label, state_layer_.value());
    const color::Argb fill = enabled_.mix(disabled_container, enabled_fill);
    if (container) {
        paint_surface(canvas, r, radii, fill, elevation_.value(), scheme[color::Role::shadow]);
    } else if (color::alpha(fill) != 0) {
        canvas.fill_path(render::rounded_rect_path(r, radii), fill);
    }
    if (options_.toggle && options_.toggle->checked_overlay > 0.0 && k > 0.0) {
        canvas.fill_path(render::rounded_rect_path(r, radii), color::scale_alpha(label_color, options_.toggle->checked_overlay * k));
    }
    if (style.outline) {
        const double width = size.outline_width * std::clamp(border_.value(), 0.0, 1.0);
        if (width > 0.0) {
            const color::Argb outline = enabled_.mix(resolve(scheme, style.disabled_outline), scheme[*style.outline]);
            paint_outline(canvas, r, radii, width, outline);
        }
    }

    if (corners_) {
        focus_.paint(canvas, scheme, r, radii);
    } else {
        focus_.paint(canvas, scheme, r, radii.top_left);
    }

    icons::IconStyle icon_style = options_.icon_style;
    if (options_.toggle && options_.toggle->fill_checked_icon && checked()) {
        icon_style.fill = 1.0;
    }
    double x = r.x + (r.width - content_width()) / 2.0 + content_offset();
    if (options_.leading_icon) {
        const double icon_size = this->icon_size() * icon_scale_;
        const double y = r.y + (r.height - icon_size) / 2.0;
        const double fade = std::clamp(icon_fade_.value(), 0.0, 1.0);
        const bool rotated = icon_rotation_ != 0.0;
        if (rotated) {
            canvas.save();
            canvas.translate(x + icon_size / 2.0, y + icon_size / 2.0);
            canvas.rotate(icon_rotation_);
            canvas.translate(-(x + icon_size / 2.0), -(y + icon_size / 2.0));
        }
        if (previous_icon_ && fade < 1.0) {
            draw_icon(canvas, *previous_icon_, x, y, icon_size, label_color, 1.0 - fade, icon_style);
        }
        draw_icon(canvas, *options_.leading_icon, x, y, icon_size, label_color, fade, icon_style);
        if (rotated) {
            canvas.restore();
        }
        x += icon_size + size.icon_label_space * label_scale_;
    }
    if (options_.label.empty()) {
        return;
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

void Button::paint_outline(render::Canvas& canvas, const render::Rect& r, const render::CornerRadii& radii, double width,
                           color::Argb color) const {
    auto shrink = [width](double v) { return std::max(0.0, v - width); };
    render::Path ring = render::rounded_rect_path(r, radii);
    const render::Rect inner{r.x + width, r.y + width, r.width - 2.0 * width, r.height - 2.0 * width};
    ring.append(render::rounded_rect_path(
        inner, {shrink(radii.top_left), shrink(radii.top_right), shrink(radii.bottom_right), shrink(radii.bottom_left)}));
    canvas.fill_path(ring, color, render::FillRule::even_odd);
}

void Button::draw_icon(render::Canvas& canvas, const icons::Icon& icon, double x, double y, double size, color::Argb color,
                       double opacity, const icons::IconStyle& style) const {
    if (opacity <= 0.0) {
        return;
    }
    icon.font.draw(canvas, icon.symbol, {x, y}, size, opacity < 1.0 ? color::scale_alpha(color, opacity) : color,
                   style);
}

}
