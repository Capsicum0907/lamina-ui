#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include "lamina/motion/tween.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/elevation.hpp"

namespace lamina::widgets {

enum class Interaction { hover, focus, press };

class InteractionElevation {
public:
    explicit InteractionElevation(const tokens::StateElevations& levels, bool enabled = true)
        : levels_(levels), elevation_(enabled ? levels.resting : levels.disabled), enabled_(enabled) {}

    void set_levels(const tokens::StateElevations& levels) {
        levels_ = levels;
        elevation_.snap_to(target());
    }

    void restyle(const tokens::StateElevations& levels) {
        levels_ = levels;
        const tokens::ElevationTweens& t = current_style().elevation.tweens;
        elevation_.retime(t.*running_);
        if (target() != elevation_.target()) {
            animate(t, &tokens::ElevationTweens::incoming);
        }
    }

    void update(bool hovered, bool focused, bool pressed, bool enabled) {
        const std::optional<Interaction> before = last();
        track(Interaction::hover, hovered);
        track(Interaction::focus, focused);
        track(Interaction::press, pressed);
        const bool was_enabled = enabled_;
        enabled_ = enabled;
        const std::optional<Interaction> now = last();
        if (!enabled) {
            elevation_.snap_to(levels_.disabled);
            return;
        }
        if (!was_enabled) {
            elevation_.snap_to(target());
            return;
        }
        if (now == before) {
            return;
        }
        const tokens::ElevationTweens& t = current_style().elevation.tweens;
        if (now) {
            animate(t, &tokens::ElevationTweens::incoming);
        } else if (before) {
            animate(t, *before == Interaction::hover ? &tokens::ElevationTweens::hover_outgoing : &tokens::ElevationTweens::outgoing);
        } else {
            elevation_.snap_to(target());
        }
    }

    void step(double dt_seconds) { elevation_.step(dt_seconds); }
    bool settled() const { return elevation_.settled(); }
    double value() const { return elevation_.value(); }
    double target() const {
        if (!enabled_) {
            return levels_.disabled;
        }
        switch (last().value_or(Interaction::hover)) {
        case Interaction::press:
            return levels_.pressed;
        case Interaction::focus:
            return levels_.focused;
        case Interaction::hover:
            return last() ? levels_.hovered : levels_.resting;
        }
        return levels_.resting;
    }

private:
    using TweenRole = motion::TweenSpec tokens::ElevationTweens::*;

    void animate(const tokens::ElevationTweens& tweens, TweenRole role) {
        if (target() == elevation_.target()) {
            return;
        }
        running_ = role;
        elevation_.animate_to(target(), tweens.*role);
    }
    std::optional<Interaction> last() const { return active_.empty() ? std::nullopt : std::optional<Interaction>(active_.back()); }
    void track(Interaction i, bool on) {
        const auto it = std::find(active_.begin(), active_.end(), i);
        if (on && it == active_.end()) {
            active_.push_back(i);
        } else if (!on && it != active_.end()) {
            active_.erase(it);
        }
    }

    tokens::StateElevations levels_;
    motion::Tweened elevation_;
    TweenRole running_ = &tokens::ElevationTweens::incoming;
    bool enabled_;
    std::vector<Interaction> active_;
};

}
