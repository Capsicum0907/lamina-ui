#pragma once

namespace lamina::motion {

struct SpringSpec {
    double damping_ratio;
    double stiffness;
};

struct SpringState {
    double value;
    double velocity;
};

struct SettleThreshold {
    double displacement;
    double velocity;
};

inline constexpr SettleThreshold default_settle_threshold{0.01, 0.01};

SpringState advance(const SpringSpec& spec, SpringState state, double target, double dt_seconds);

bool is_settled(SpringState state, double target, SettleThreshold threshold = default_settle_threshold);

}
