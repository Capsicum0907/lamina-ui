#pragma once

#include "lamina/input/events.hpp"
#include "lamina/motion/spline_decay.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/drag.hpp"

namespace lamina::widgets {

enum class TouchScrollStep { pass, claim, drag, release, caught };

struct TouchScrollUpdate {
    TouchScrollStep step = TouchScrollStep::pass;
    double dx = 0.0;
    double dy = 0.0;
    double velocity_x = 0.0;
    double velocity_y = 0.0;
};

const motion::SplineDecay& fling_decay();

bool angle_aligned(const tokens::TouchScrollTokens& tokens, input::ScrollAxes axis, double angle_degrees);

class TouchScroll {
public:
    explicit TouchScroll(const tokens::TouchScrollTokens& tokens = current_style().gesture.touch_scroll);

    TouchScrollUpdate press(const input::PointerEvent& event, input::ScrollAxes axes, bool catch_fling);
    TouchScrollUpdate move(const input::PointerEvent& event);
    TouchScrollUpdate release(const input::PointerEvent& event);
    void yield();
    void cancel();

    bool pending() const { return phase_ == Phase::pending; }
    bool owns() const { return phase_ == Phase::owning; }
    input::ScrollAxes axes() const { return axes_; }
    input::PointerEvent forward(const input::PointerEvent& event) const;
    const tokens::TouchScrollTokens& tokens() const { return *tokens_; }

private:
    enum class Phase { idle, pending, owning };

    void track(const input::PointerEvent& event);

    const tokens::TouchScrollTokens* tokens_;
    Phase phase_ = Phase::idle;
    input::ScrollAxes axes_;
    double press_x_ = 0.0;
    double press_y_ = 0.0;
    double last_x_ = 0.0;
    double last_y_ = 0.0;
    motion::VelocityTracker tracker_x_;
    motion::VelocityTracker tracker_y_;
};

}
