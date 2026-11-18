#pragma once

#include "lamina/tokens/motion.hpp"

namespace lamina::motion {

struct SpringState {
    double value;
    double velocity;
};

struct SettleThreshold {
    double displacement;
    double velocity;
};

inline constexpr SettleThreshold default_settle_threshold{0.01, 0.01};

SpringState advance(const tokens::SpringSpec& spec, SpringState state, double target, double dt_seconds);

bool is_settled(SpringState state, double target, SettleThreshold threshold = default_settle_threshold);

}
