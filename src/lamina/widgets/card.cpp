#include "lamina/widgets/card.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::CardTokens& t = tokens::card_tokens;

}

Card::Card(CardOptions options)
    : options_(std::move(options)),
      enabled_(options_.enabled),
      elevation_(tokens::card_style(options_.style).elevation, options_.enabled),
      state_layer_(0.0, t.state_layer_spring) {
    if (options_.clickable) {
        options_.accessible_name = required_name(std::move(options_.accessible_name), "A clickable card");
    }
    if (!options_.enabled && options_.widgets) {
        options_.widgets->set_enabled(false);
    }
}

layout::Size Card::measure(const layout::Constraints& c) {
    layout::Size s{c.min_width, c.min_height};
    if (options_.content) {
        s = options_.content->measure(c);
    }
    if (options_.clickable) {
        s.width = std::max(s.width, tokens::minimum_interactive_size);
        s.height = std::max(s.height, tokens::minimum_interactive_size);
    }
    return c.constrain(s);
}

void Card::arrange(const render::Rect& r) {
    rect_ = r;
    if (options_.content) {
        options_.content->arrange(r);
    }
}

double Card::radius() const {
    return tokens::corner_radius(t.corner, rect_.width, rect_.height);
}

AccessibleNode Card::accessible() const {
    AccessibleNode node{options_.clickable ? AccessibleRole::button : AccessibleRole::group, options_.accessible_name, rect_,
                        {.enabled = enabled(), .focusable = can_focus(), .focused = focused()}};
    if (options_.widgets) {
        node.children = options_.widgets->accessible_nodes();
    }
    return node;
}

bool Card::over_content_control(double x, double y) const {
    if (!options_.widgets) {
        return false;
    }
    bool over = false;
    visit_accessible(options_.widgets->accessible_nodes(), [&](const AccessibleNode& n) {
        if (n.state.focusable && n.state.enabled && contains(n.bounds, x, y)) {
            over = true;
        }
    });
    return over;
}

void Card::retarget() {
    if (!options_.clickable) {
        return;
    }
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
    elevation_.update(press_.hovered, focus_.focused(), press_.pressed, enabled());
}

void Card::set_focused(bool focused) {
    focus_.set_focused(focused && can_focus());
    retarget();
}

void Card::set_enabled(bool enabled) {
    if (options_.widgets) {
        options_.widgets->set_enabled(enabled);
    }
    if (!enabled_.set(enabled)) {
        return;
    }
    if (!enabled) {
        press_ = {};
        focus_.set_focused(false);
    }
    if (options_.clickable) {
        retarget();
    } else {
        elevation_.update(false, false, false, enabled);
    }
}

bool Card::holds_pointer() const {
    return options_.widgets && options_.widgets->holds_pointer();
}

bool Card::handle(const input::PointerEvent& e) {
    if (options_.widgets) {
        options_.widgets->handle(e);
    }
    focus_.release_on_press(e);
    if (!options_.clickable || !enabled()) {
        return false;
    }
    input::PointerEvent own = e;
    if (e.action != input::PointerAction::leave && over_content_control(e.x_dp, e.y_dp)) {
        own.action = input::PointerAction::leave;
    }
    const bool clicked = press_.update(own, touch_target(rect_));
    retarget();
    return clicked;
}

bool Card::handle(const input::KeyEvent& e) {
    if (options_.widgets) {
        options_.widgets->handle(e);
    }
    return can_focus() && focus_.activates(e, KeyActivation::space_or_enter);
}

void Card::cancel_interaction() {
    press_ = {};
    if (options_.widgets) {
        options_.widgets->cancel_interaction();
    }
    retarget();
}

void Card::step(double dt) {
    focus_.step(dt);
    enabled_.step(dt);
    elevation_.step(dt);
    state_layer_.step(dt);
    if (options_.widgets) {
        options_.widgets->step(dt);
    }
}

bool Card::animating() const {
    return !focus_.settled() || !enabled_.settled() || !elevation_.settled() || !state_layer_.settled() ||
           (options_.widgets && options_.widgets->animating());
}

void Card::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const tokens::CardStyleTokens& s = tokens::card_style(options_.style);
    const double r = radius();
    const color::Argb container = scheme[s.container];
    const color::Argb enabled_fill = color::state_layer(container, scheme[t.state_layer], state_layer_.value());
    const color::Argb fill = enabled_.mix(resolve(scheme, s.disabled_container), enabled_fill);
    paint_surface(canvas, rect_, r, fill, elevation_.value(), scheme[color::Role::shadow]);
    if (options_.widgets) {
        canvas.push_clip(render::rounded_rect_path(rect_, render::uniform_radii(r)));
        const double content = motion::lerp(s.disabled_content.opacity, 1.0, enabled_.amount());
        if (content < 1.0) {
            options_.widgets->paint(canvas, color::faded(scheme, content));
        } else {
            options_.widgets->paint(canvas, scheme);
        }
        canvas.pop_clip();
    }
    if (s.outline) {
        const color::Argb outline = enabled_.mix(resolve(scheme, s.disabled_outline), scheme[*s.outline]);
        canvas.fill_path(render::ring_path(rect_, r, t.outline_width), outline, render::FillRule::even_odd);
    }
    focus_.paint(canvas, scheme, rect_, r);
}

}
