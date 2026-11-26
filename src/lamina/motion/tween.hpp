#pragma once

#include <algorithm>

#include "lamina/motion/easing.hpp"
#include "lamina/motion/lerp.hpp"

namespace lamina::motion {

struct TweenSpec {
    double duration_seconds;
    CubicBezier easing;

    bool operator==(const TweenSpec&) const = default;
};

inline double retimed_elapsed(const TweenSpec& running, double elapsed_seconds, const TweenSpec& next) {
    if (running == next) {
        return elapsed_seconds;
    }
    if (next.duration_seconds <= 0.0) {
        return next.duration_seconds;
    }
    const double linear = running.duration_seconds <= 0.0 ? 1.0 : std::clamp(elapsed_seconds / running.duration_seconds, 0.0, 1.0);
    const double progress = running.easing == next.easing ? linear : ease_inverse(next.easing, ease(running.easing, linear));
    return progress * next.duration_seconds;
}

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
    void retime(const TweenSpec& spec) {
        if (settled()) {
            return;
        }
        elapsed_ = retimed_elapsed(spec_, elapsed_, spec);
        spec_ = spec;
        if (spec.duration_seconds <= 0.0) {
            snap_to(to_);
        }
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
    double origin() const { return from_; }
    double elapsed() const { return elapsed_; }
    const TweenSpec& spec() const { return spec_; }
    bool settled() const { return elapsed_ >= spec_.duration_seconds || from_ == to_; }

private:
    double from_;
    double to_;
    double value_;
    double elapsed_ = 0.0;
    TweenSpec spec_{0.0, {0.0, 0.0, 1.0, 1.0}};
};

}
