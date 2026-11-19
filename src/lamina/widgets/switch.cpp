#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/switch.hpp"

#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::SwitchTokens& t = tokens::switch_tokens;
using tokens::motion_scheme;

constexpr double travel = t.track_width - t.track_height;

}

Switch::Switch(SwitchOptions options)
    : options_(std::move(options)),
      offset_(options_.checked ? travel : 0.0, motion_scheme.fast_spatial()),
      thumb_size_(0.0, motion_scheme.fast_spatial()),
      state_layer_(0.0, motion_scheme.fast_effects()),
      checked_(options_.checked ? 1.0 : 0.0, motion_scheme.default_effects()) {
    thumb_size_.snap_to(target_thumb_size());
}

double Switch::width() const {
    return t.track_width;
}

double Switch::height() const {
    return t.track_height;
}

render::Rect Switch::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

double Switch::thumb_center_x() const {
    return origin_.x + t.track_height / 2.0 + offset_.value();
}

const std::optional<icons::Icon>& Switch::current_icon() const {
    return options_.checked ? options_.checked_icon : options_.unchecked_icon;
}

double Switch::target_thumb_size() const {
    if (press_.pressed) {
        return t.pressed_thumb;
    }
    return current_icon() || options_.checked ? t.checked_thumb : t.unchecked_thumb;
}

void Switch::retarget() {
    offset_.animate_to(options_.checked ? travel : 0.0);
    thumb_size_.animate_to(target_thumb_size());
    state_layer_.animate_to(press_.state_layer_opacity());
    checked_.animate_to(options_.checked ? 1.0 : 0.0);
}

void Switch::set_checked(bool checked) {
    options_.checked = checked;
    retarget();
}

bool Switch::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, touch_target(bounds()));
    if (clicked) {
        options_.checked = !options_.checked;
    }
    retarget();
    return clicked;
}

void Switch::cancel_interaction() {
    press_ = {};
    retarget();
}

void Switch::step(double dt) {
    offset_.step(dt);
    thumb_size_.step(dt);
    state_layer_.step(dt);
    checked_.step(dt);
}

bool Switch::animating() const {
    return !offset_.settled() || !thumb_size_.settled() || !state_layer_.settled() || !checked_.settled();
}

void Switch::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const double p = checked_.value();
    const double track_radius = r.height / 2.0;

    canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(track_radius)),
                     color::mix(scheme[t.unchecked.track], scheme[t.checked.track], p));
    const color::Argb border = mix_optional(scheme, t.unchecked.border, t.checked.border, p);
    if (color::alpha(border) != 0) {
        canvas.fill_path(ring_path(r, track_radius, t.track_outline_width), border, render::FillRule::even_odd);
    }

    const render::Point center{thumb_center_x(), r.y + r.height / 2.0};
    const double size = std::max(0.0, thumb_size_.value());
    canvas.fill_path(circle_path(center, size / 2.0), color::mix(scheme[t.unchecked.thumb], scheme[t.checked.thumb], p));
    if (state_layer_.value() > 0.0) {
        canvas.fill_path(circle_path(center, t.state_layer_size / 2.0),
                         color::with_alpha(scheme[t.state_layer], state_layer_.value()));
    }
    if (const auto& icon = current_icon()) {
        const color::Argb icon_color = color::mix(scheme[t.unchecked.icon], scheme[t.checked.icon], p);
        icons::draw_centered(canvas, *icon, center, t.icon_size, icon_color);
    }
}

}
