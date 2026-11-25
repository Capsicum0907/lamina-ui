#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/fab.hpp"

#include <utility>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {


}

Fab::Fab(FabOptions options)
    : options_(std::move(options)), state_layer_(0.0, current_style().motion.fast_effects()), elevation_(0.0, current_style().motion.default_effects()) {
    options_.accessible_name = required_name(std::move(options_.accessible_name), "A FAB");
    elevation_.snap_to(state_elevation(current_style().fab.style(options_.style), press_));
}

double Fab::width() const {
    return current_style().fab.size(options_.size).size;
}

double Fab::height() const {
    return current_style().fab.size(options_.size).size;
}

render::Rect Fab::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

void Fab::set_focused(bool focused) {
    focus_.set_focused(focused);
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

bool Fab::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool clicked = press_.update(e, bounds());
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    elevation_.animate_to(state_elevation(current_style().fab.style(options_.style), press_));
    return clicked;
}

bool Fab::handle(const input::KeyEvent& e) {
    return focus_.activates(e, KeyActivation::space_or_enter);
}

void Fab::cancel_interaction() {
    press_ = {};
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    elevation_.animate_to(state_elevation(current_style().fab.style(options_.style), press_));
}

void Fab::restyle() {
    style_stamp_.refresh();
    const auto& motion = current_style().motion;
    state_layer_.set_spec(motion.fast_effects());
    elevation_.set_spec(motion.default_effects());
    focus_.restyle();
    elevation_.animate_to(state_elevation(current_style().fab.style(options_.style), press_));
}

void Fab::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    state_layer_.step(dt);
    elevation_.step(dt);
    focus_.step(dt);
}

bool Fab::animating() const {
    return !state_layer_.settled() || !elevation_.settled() || !focus_.settled();
}

void Fab::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& size = current_style().fab.size(options_.size);
    const auto& style = current_style().fab.style(options_.style);
    const render::Rect r = bounds();
    const double radius = current_style().shape.radius(size.corner, r.width, r.height);
    const color::Argb content = scheme[style.content];
    const color::Argb fill = color::state_layer(scheme[style.container], content, state_layer_.value());
    paint_surface(canvas, r, radius, fill, elevation_.value(), scheme[color::Role::shadow]);
    focus_.paint(canvas, scheme, r, radius);
    icons::draw_centered(canvas, options_.icon, r, size.icon_size, content);
}

}
