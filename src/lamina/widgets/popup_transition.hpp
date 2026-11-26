#pragma once

#include <algorithm>

#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

class PopupTransition {
public:
    PopupTransition(double closed_scale, motion::SpringSpec scale_spring, motion::SpringSpec alpha_spring)
        : closed_scale_(closed_scale), scale_(closed_scale, scale_spring), alpha_(0.0, alpha_spring) {}

    bool open() {
        const bool was_hidden = !visible();
        if (was_hidden) {
            scale_.snap_to(closed_scale_);
            alpha_.snap_to(0.0);
        }
        open_ = true;
        scale_.animate_to(1.0);
        alpha_.animate_to(1.0);
        return was_hidden;
    }

    void close() {
        open_ = false;
        scale_.animate_to(closed_scale_);
        alpha_.animate_to(0.0);
    }

    void restyle(double closed_scale, motion::SpringSpec scale_spring, motion::SpringSpec alpha_spring) {
        closed_scale_ = closed_scale;
        scale_.set_spec(scale_spring);
        alpha_.set_spec(alpha_spring);
        if (!open_) {
            scale_.animate_to(closed_scale_);
        }
    }

    bool is_open() const { return open_; }
    bool visible() const { return open_ || !alpha_.settled() || alpha_.value() > 0.0; }
    double opacity() const { return std::clamp(alpha_.value(), 0.0, 1.0); }
    double scale() const { return scale_.value(); }

    bool dismisses(const input::PointerEvent& e, const render::Rect& bounds) const {
        return open_ && e.action == input::PointerAction::down && !contains(bounds, e.x_dp, e.y_dp);
    }
    bool dismisses(const input::KeyEvent& e) const { return open_ && e.key == input::Key::escape; }

    void step(double dt_seconds) {
        scale_.step(dt_seconds);
        alpha_.step(dt_seconds);
    }
    bool settled() const { return scale_.settled() && alpha_.settled(); }

    void scale_about(render::Canvas& canvas, render::Point pivot) const {
        canvas.translate(pivot.x, pivot.y);
        canvas.scale(scale_.value());
        canvas.translate(-pivot.x, -pivot.y);
    }

private:
    double closed_scale_;
    bool open_ = false;
    motion::Animated scale_;
    motion::Animated alpha_;
};

}
