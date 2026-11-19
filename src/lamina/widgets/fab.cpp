#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/fab.hpp"

#include <utility>

#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr motion::SpringSpec elevation_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr motion::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

}

Fab::Fab(FabOptions options)
    : options_(std::move(options)), state_layer_(0.0, state_spring), elevation_(0.0, elevation_spring) {
    elevation_.snap_to(state_elevation(tokens::fab_style(options_.style), press_));
}

double Fab::width() const {
    return tokens::fab_size(options_.size).size;
}

double Fab::height() const {
    return tokens::fab_size(options_.size).size;
}

render::Rect Fab::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

bool Fab::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, bounds());
    state_layer_.animate_to(press_.state_layer_opacity());
    elevation_.animate_to(state_elevation(tokens::fab_style(options_.style), press_));
    return clicked;
}

void Fab::step(double dt) {
    state_layer_.step(dt);
    elevation_.step(dt);
}

bool Fab::animating() const {
    return !state_layer_.settled() || !elevation_.settled();
}

void Fab::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& size = tokens::fab_size(options_.size);
    const auto& style = tokens::fab_style(options_.style);
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(size.corner, r.width, r.height);
    const color::Argb content = scheme[style.content];
    const color::Argb fill = with_state_layer(scheme[style.container], content, state_layer_.value());
    paint_surface(canvas, r, radius, fill, elevation_.value(), scheme[color::Role::shadow]);
    icons::draw_centered(canvas, options_.icon, r, size.icon_size, content);
}

}
