#pragma once

#include <algorithm>

#include "lamina/motion/spring.hpp"

namespace lamina::motion {

class Animated {
public:
    Animated(double value, motion::SpringSpec spec) : state_{value, 0.0}, target_(value), spec_(spec) {}

    void animate_to(double target) { target_ = target; }
    void snap_to(double value) {
        state_ = {value, 0.0};
        target_ = value;
    }
    void set_spec(motion::SpringSpec spec) { spec_ = spec; }

    void step(double dt_seconds) {
        if (settled()) {
            state_ = {target_, 0.0};
            return;
        }
        state_ = advance(spec_, state_, target_, dt_seconds);
    }

    double value() const { return state_.value; }
    double target() const { return target_; }
    bool settled(SettleThreshold threshold = default_settle_threshold) const {
        return is_settled(state_, target_, threshold);
    }

private:
    SpringState state_;
    double target_;
    motion::SpringSpec spec_;
};

template <typename Items, typename... Members>
bool any_unsettled(const Items& items, Members... members) {
    return std::any_of(items.begin(), items.end(), [&](const auto& item) { return (!(item.*members).settled() || ...); });
}

}
