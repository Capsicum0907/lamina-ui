#include "lamina/widgets/fab.hpp"

#include <utility>

#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/paint.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr tokens::SpringSpec elevation_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

}

Fab::Fab(FabOptions options)
    : options_(std::move(options)), state_layer_(0.0, state_spring), elevation_(0.0, elevation_spring) {
    elevation_.snap_to(target_elevation());
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

double Fab::target_elevation() const {
    const auto& style = tokens::fab_style(options_.style);
    const std::size_t level = press_.pressed   ? style.pressed_elevation
                              : press_.hovered ? style.hovered_elevation
                                               : style.elevation;
    return tokens::elevation_dp[level];
}

bool Fab::handle(const input::PointerEvent& e) {
    const bool clicked = press_.update(e, bounds());
    state_layer_.animate_to(press_.state_layer_opacity());
    elevation_.animate_to(target_elevation());
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
    options_.icon.font.draw(canvas, options_.icon.symbol,
                            {r.x + (r.width - size.icon_size) / 2.0, r.y + (r.height - size.icon_size) / 2.0},
                            size.icon_size, content);
}

}
