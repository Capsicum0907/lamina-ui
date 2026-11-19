#include "lamina/widgets/radio_button.hpp"

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::RadioButtonTokens& t = tokens::radio_button_tokens;

constexpr tokens::SpringSpec dot_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
constexpr tokens::SpringSpec color_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

double target_dot(bool selected) {
    return selected ? t.dot_size / 2.0 : 0.0;
}

}

RadioButton::RadioButton(RadioButtonOptions options)
    : options_(options),
      dot_radius_(target_dot(options_.selected), dot_spring),
      selected_(options_.selected ? 1.0 : 0.0, color_spring),
      state_layer_(0.0, state_spring) {}

double RadioButton::width() const {
    return t.state_layer_size;
}

double RadioButton::height() const {
    return t.state_layer_size;
}

render::Rect RadioButton::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

void RadioButton::set_selected(bool selected) {
    options_.selected = selected;
    dot_radius_.animate_to(target_dot(selected));
    selected_.animate_to(selected ? 1.0 : 0.0);
}

bool RadioButton::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, touch_target(bounds()));
    if (clicked) {
        set_selected(true);
    }
    state_layer_.animate_to(press_.state_layer_opacity());
    return clicked;
}

void RadioButton::step(double dt) {
    dot_radius_.step(dt);
    selected_.step(dt);
    state_layer_.step(dt);
}

bool RadioButton::animating() const {
    return !dot_radius_.settled() || !selected_.settled() || !state_layer_.settled();
}

void RadioButton::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    const render::Point center{r.x + r.width / 2.0, r.y + r.height / 2.0};
    const color::Argb c = color::mix(scheme[t.unselected], scheme[t.selected], selected_.value());
    const double outer = t.icon_size / 2.0;
    render::Path ring = circle_path(center, outer);
    ring.append(circle_path(center, outer - t.stroke_width));
    canvas.fill_path(ring, c, render::FillRule::even_odd);
    const double dot = dot_radius_.value() - t.stroke_width / 2.0;
    if (dot > 0.0) {
        canvas.fill_path(circle_path(center, dot), c);
    }
    if (state_layer_.value() > 0.0) {
        canvas.fill_path(circle_path(center, t.state_layer_size / 2.0),
                         color::with_alpha(scheme[t.state_layer], state_layer_.value()));
    }
}

}
