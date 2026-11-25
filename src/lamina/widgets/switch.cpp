#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/switch.hpp"

#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"

namespace lamina::widgets {

namespace {

const tokens::SwitchTokens& t() {
    return current_style().switch_;
}

double travel() {
    return t().track_width - t().track_height;
}

}

Switch::Switch(SwitchOptions options)
    : options_(std::move(options)),
      enabled_(options_.enabled),
      offset_(options_.checked ? travel() : 0.0, current_style().motion.fast_spatial()),
      thumb_size_(0.0, current_style().motion.fast_spatial()),
      state_layer_(0.0, current_style().motion.fast_effects()),
      checked_(options_.checked ? 1.0 : 0.0, current_style().motion.default_effects()) {
    thumb_size_.snap_to(target_thumb_size());
}

double Switch::width() const {
    return t().track_width;
}

double Switch::height() const {
    return t().track_height;
}

render::Rect Switch::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

double Switch::thumb_center_x() const {
    return origin_.x + t().track_height / 2.0 + offset_.value();
}

const std::optional<icons::Icon>& Switch::current_icon() const {
    return options_.checked ? options_.checked_icon : options_.unchecked_icon;
}

double Switch::target_thumb_size() const {
    if (press_.pressed) {
        return t().pressed_thumb;
    }
    return current_icon() || options_.checked ? t().checked_thumb : t().unchecked_thumb;
}

void Switch::retarget() {
    offset_.animate_to(options_.checked ? travel() : 0.0);
    thumb_size_.animate_to(target_thumb_size());
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    checked_.animate_to(options_.checked ? 1.0 : 0.0);
}

void Switch::set_checked(bool checked) {
    options_.checked = checked;
    retarget();
}

void Switch::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    retarget();
}

void Switch::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    retarget();
}

std::optional<TargetHit> Switch::target_hit(const input::PointerEvent& e) const {
    return widgets::target_hit(bounds(), e.x_dp, e.y_dp);
}

bool Switch::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (!enabled()) {
        return false;
    }
    const bool clicked = press_.update(e, hit_area(e, bounds()));
    if (clicked) {
        options_.checked = !options_.checked;
    }
    retarget();
    return clicked;
}

bool Switch::handle(const input::KeyEvent& e) {
    if (!enabled() || !focus_.activates(e, KeyActivation::space)) {
        return false;
    }
    options_.checked = !options_.checked;
    retarget();
    return true;
}

void Switch::cancel_interaction() {
    press_ = {};
    retarget();
}

void Switch::restyle() {
    style_stamp_.refresh();
    const auto& motion = current_style().motion;
    offset_.set_spec(motion.fast_spatial());
    thumb_size_.set_spec(motion.fast_spatial());
    state_layer_.set_spec(motion.fast_effects());
    checked_.set_spec(motion.default_effects());
    focus_.restyle();
    enabled_.restyle();
    retarget();
}

void Switch::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    offset_.step(dt);
    thumb_size_.step(dt);
    state_layer_.step(dt);
    checked_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
}

bool Switch::animating() const {
    return !offset_.settled() || !thumb_size_.settled() || !state_layer_.settled() || !checked_.settled() || !focus_.settled() ||
           !enabled_.settled();
}

void Switch::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const double p = checked_.value();
    const double track_radius = r.height / 2.0;

    const auto& off = t().disabled_unchecked;
    const auto& on = t().disabled_checked;
    auto both = [&](const tokens::StateColor& d_off, const tokens::StateColor& d_on, color::Role e_off, color::Role e_on) {
        return enabled_.mix(color::mix(resolve(scheme, d_off), resolve(scheme, d_on), p), color::mix(scheme[e_off], scheme[e_on], p));
    };
    canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(track_radius)),
                     both(off.track, on.track, t().unchecked.track, t().checked.track));
    const color::Argb border =
        enabled_.mix(mix_resolved(scheme, off.border, on.border, p), mix_optional(scheme, t().unchecked.border, t().checked.border, p));
    if (color::alpha(border) != 0) {
        canvas.fill_path(render::ring_path(r, track_radius, t().track_outline_width), border, render::FillRule::even_odd);
    }

    focus_.paint(canvas, scheme, r, track_radius);

    const render::Point center{thumb_center_x(), r.y + r.height / 2.0};
    const double size = std::max(0.0, thumb_size_.value());
    canvas.fill_path(render::circle_path(center, size / 2.0), both(off.thumb, on.thumb, t().unchecked.thumb, t().checked.thumb));
    if (state_layer_.value() > 0.0) {
        canvas.fill_path(render::circle_path(center, t().state_layer_size / 2.0),
                         color::with_alpha(scheme[t().state_layer], state_layer_.value()));
    }
    if (const auto& icon = current_icon()) {
        const color::Argb icon_color = both(off.icon, on.icon, t().unchecked.icon, t().checked.icon);
        icons::draw_centered(canvas, *icon, center, t().icon_size, icon_color);
    }
}

}
