#pragma once

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/focus.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

enum class KeyActivation { space, space_or_enter };

constexpr bool activates(const input::KeyEvent& e, KeyActivation keys) {
    if (e.ctrl || e.repeat) {
        return false;
    }
    return e.key == input::Key::space || (keys == KeyActivation::space_or_enter && e.key == input::Key::enter);
}

class FocusIndicator {
public:
    FocusIndicator() : amount_(0.0, tokens::motion_scheme.fast_effects()) {}

    bool focused() const { return focused_; }
    void set_focused(bool focused) {
        focused_ = focused;
        amount_.animate_to(focused ? 1.0 : 0.0);
    }
    void release_on_press(const input::PointerEvent& e) {
        if (e.action == input::PointerAction::down) {
            set_focused(false);
        }
    }
    bool activates(const input::KeyEvent& e, KeyActivation keys) const { return focused_ && widgets::activates(e, keys); }
    double state_layer_opacity(double pointer_opacity) const {
        return focused_ ? std::max(pointer_opacity, tokens::state_layer_opacity::focus) : pointer_opacity;
    }

    void step(double dt_seconds) { amount_.step(dt_seconds); }
    bool settled() const { return amount_.settled(); }
    double amount() const { return std::clamp(amount_.value(), 0.0, 1.0); }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& shape, double radius) const {
        const double a = amount();
        if (a <= 0.0) {
            return;
        }
        const tokens::FocusIndicatorTokens& t = tokens::focus_indicator;
        const double grow = t.outer_offset + t.thickness;
        const render::Rect outer{shape.x - grow, shape.y - grow, shape.width + 2.0 * grow, shape.height + 2.0 * grow};
        canvas.fill_path(render::ring_path(outer, radius + grow, t.thickness), color::scale_alpha(scheme[t.color], a),
                         render::FillRule::even_odd);
    }
    void paint_inward(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& shape, double radius) const {
        const double a = amount();
        if (a <= 0.0) {
            return;
        }
        const tokens::FocusIndicatorTokens& t = tokens::focus_indicator;
        const double d = t.inward_offset;
        const render::Rect outer{shape.x + d, shape.y + d, shape.width - 2.0 * d, shape.height - 2.0 * d};
        canvas.fill_path(render::ring_path(outer, std::max(0.0, radius - d), t.thickness), color::scale_alpha(scheme[t.color], a),
                         render::FillRule::even_odd);
    }
    void paint_circle(render::Canvas& canvas, const color::ColorScheme& scheme, render::Point center, double radius) const {
        paint(canvas, scheme, render::circle_rect(center, radius), radius);
    }

private:
    bool focused_ = false;
    motion::Animated amount_;
};

}
