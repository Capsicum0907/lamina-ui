#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/radio_button.hpp"

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

const tokens::RadioButtonTokens& t() {
    return current_style().radio_button;
}

double target_dot(bool selected) {
    return selected ? t().dot_size / 2.0 : 0.0;
}

}

RadioButton::RadioButton(RadioButtonOptions options)
    : options_(options),
      enabled_(options_.enabled),
      dot_radius_(target_dot(options_.selected), current_style().motion.fast_spatial()),
      selected_(options_.selected ? 1.0 : 0.0, current_style().motion.default_effects()),
      state_layer_(0.0, current_style().motion.fast_effects()) {}

double RadioButton::width() const {
    return t().state_layer_size;
}

double RadioButton::height() const {
    return t().state_layer_size;
}

render::Rect RadioButton::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

void RadioButton::set_selected(bool selected) {
    options_.selected = selected;
    dot_radius_.animate_to(target_dot(selected));
    selected_.animate_to(selected ? 1.0 : 0.0);
}

void RadioButton::set_focused(bool focused) {
    focus_.set_focused(focused && enabled());
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

void RadioButton::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

std::optional<TargetHit> RadioButton::target_hit(const input::PointerEvent& e) const {
    return widgets::target_hit(bounds(), e.x_dp, e.y_dp);
}

bool RadioButton::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (!enabled()) {
        return false;
    }
    const bool clicked = press_.update(e, hit_area(e, bounds()));
    if (clicked) {
        set_selected(true);
    }
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    return clicked;
}

bool RadioButton::handle(const input::KeyEvent& e) {
    if (!enabled() || !focus_.activates(e, KeyActivation::space)) {
        return false;
    }
    set_selected(true);
    return true;
}

void RadioButton::cancel_interaction() {
    press_ = {};
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

void RadioButton::restyle() {
    style_stamp_.refresh();
    const auto& motion = current_style().motion;
    dot_radius_.set_spec(motion.fast_spatial());
    selected_.set_spec(motion.default_effects());
    state_layer_.set_spec(motion.fast_effects());
    focus_.restyle();
    enabled_.restyle();
    dot_radius_.animate_to(target_dot(options_.selected));
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

void RadioButton::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    dot_radius_.step(dt);
    selected_.step(dt);
    state_layer_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
}

bool RadioButton::animating() const {
    return !dot_radius_.settled() || !selected_.settled() || !state_layer_.settled() || !focus_.settled() || !enabled_.settled();
}

void RadioButton::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const render::Point center{r.x + r.width / 2.0, r.y + r.height / 2.0};
    const double s = selected_.value();
    const color::Argb c = enabled_.mix(color::mix(resolve(scheme, t().disabled_unselected), resolve(scheme, t().disabled_selected), s),
                                       color::mix(scheme[t().unselected], scheme[t().selected], s));
    const double outer = t().icon_size / 2.0;
    render::Path ring = render::circle_path(center, outer);
    ring.append(render::circle_path(center, outer - t().stroke_width));
    canvas.fill_path(ring, c, render::FillRule::even_odd);
    const double dot = dot_radius_.value() - t().stroke_width / 2.0;
    if (dot > 0.0) {
        canvas.fill_path(render::circle_path(center, dot), c);
    }
    if (state_layer_.value() > 0.0) {
        canvas.fill_path(render::circle_path(center, t().state_layer_size / 2.0),
                         color::with_alpha(scheme[t().state_layer], state_layer_.value()));
    }
    focus_.paint_circle(canvas, scheme, center, t().state_layer_size / 2.0);
}

}
