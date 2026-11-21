#include "lamina/widgets/extended_fab.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

ExtendedFab::ExtendedFab(ExtendedFabOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      label_(text::layout_line(fonts_, options_.label, tokens::text_style(tokens::type_style(tokens::extended_fab_size(options_.size).label)))),
      width_progress_(options_.expanded ? 1.0 : 0.0, tokens::extended_fab_motion.width),
      label_alpha_(options_.expanded ? 1.0 : 0.0, tokens::extended_fab_motion.label_alpha),
      state_layer_(0.0, tokens::motion_scheme.fast_effects()),
      elevation_(0.0, tokens::motion_scheme.default_effects()) {
    options_.label = required_name(std::move(options_.label), "An extended FAB");
    elevation_.snap_to(state_elevation(tokens::fab_style(options_.style), press_));
}

double ExtendedFab::expanded_width() const {
    const auto& s = tokens::extended_fab_size(options_.size);
    return std::max(s.height, s.leading_space + s.icon_size + s.icon_label_space + label_.width + s.trailing_space);
}

double ExtendedFab::width() const {
    return motion::lerp(height(), expanded_width(), width_progress_.value());
}

void ExtendedFab::set_expanded(bool expanded) {
    options_.expanded = expanded;
    width_progress_.animate_to(expanded ? 1.0 : 0.0);
    label_alpha_.animate_to(expanded ? 1.0 : 0.0);
}

void ExtendedFab::retarget() {
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    elevation_.animate_to(state_elevation(tokens::fab_style(options_.style), press_));
}

void ExtendedFab::set_focused(bool focused) {
    focus_.set_focused(focused);
    retarget();
}

bool ExtendedFab::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool clicked = press_.update(e, bounds());
    retarget();
    return clicked;
}

bool ExtendedFab::handle(const input::KeyEvent& e) {
    return focus_.activates(e, KeyActivation::space_or_enter);
}

void ExtendedFab::cancel_interaction() {
    press_ = {};
    retarget();
}

void ExtendedFab::step(double dt) {
    width_progress_.step(dt);
    label_alpha_.step(dt);
    state_layer_.step(dt);
    elevation_.step(dt);
    focus_.step(dt);
}

bool ExtendedFab::animating() const {
    return !width_progress_.settled() || !label_alpha_.settled() || !state_layer_.settled() || !elevation_.settled() || !focus_.settled();
}

void ExtendedFab::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& s = tokens::extended_fab_size(options_.size);
    const auto& style = tokens::fab_style(options_.style);
    const render::Rect r = bounds();
    const double radius = tokens::corner_radius(s.corner, r.width, r.height);
    const color::Argb content = scheme[style.content];
    paint_surface(canvas, r, radius, color::state_layer(scheme[style.container], content, state_layer_.value()), elevation_.value(),
                  scheme[color::Role::shadow]);
    focus_.paint(canvas, scheme, r, radius);
    canvas.save();
    canvas.push_clip(render::rounded_rect_path(r, render::uniform_radii(radius)));
    const double x = r.x + s.leading_space;
    icons::draw_centered(canvas, options_.icon, render::Rect{x, r.y, s.icon_size, r.height}, s.icon_size, content);
    const double alpha = std::clamp(label_alpha_.value(), 0.0, 1.0);
    const bool fully_collapsed = !options_.expanded && label_alpha_.settled() && width_progress_.settled();
    if (!fully_collapsed && alpha > 0.0) {
        text::draw_line(canvas, fonts_, label_, {x + s.icon_size + s.icon_label_space, r.y + (r.height - label_.box_height()) / 2.0},
                        color::scale_alpha(content, alpha));
    }
    canvas.pop_clip();
    canvas.restore();
}

}
