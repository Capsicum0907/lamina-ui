#pragma once

#include <algorithm>

#include "lamina/motion/easing.hpp"
#include "lamina/motion/lerp.hpp"

namespace lamina::motion {

struct TweenSpec {
    double duration_seconds;
    CubicBezier easing;
};

class Tweened {
public:
    explicit Tweened(double value) : from_(value), to_(value), value_(value) {}

    void animate_to(double target, const TweenSpec& spec) {
        if (target == to_) {
            return;
        }
        from_ = value_;
        to_ = target;
        spec_ = spec;
        elapsed_ = 0.0;
        if (spec.duration_seconds <= 0.0) {
            snap_to(target);
        }
    }
    void snap_to(double value) {
        from_ = value;
        to_ = value;
        value_ = value;
        elapsed_ = spec_.duration_seconds;
    }
    void step(double dt_seconds) {
        if (settled()) {
            value_ = to_;
            return;
        }
        elapsed_ = std::min(spec_.duration_seconds, elapsed_ + dt_seconds);
        value_ = lerp(from_, to_, ease(spec_.easing, elapsed_ / spec_.duration_seconds));
    }

    double value() const { return value_; }
    double target() const { return to_; }
    bool settled() const { return elapsed_ >= spec_.duration_seconds || from_ == to_; }

private:
    double from_;
    double to_;
    double value_;
    double elapsed_ = 0.0;
    TweenSpec spec_{0.0, {0.0, 0.0, 1.0, 1.0}};
};

}
