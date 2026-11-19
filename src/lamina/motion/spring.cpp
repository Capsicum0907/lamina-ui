#include "lamina/motion/spring.hpp"

#include <cmath>

namespace lamina::motion {

SpringState advance(const SpringSpec& spec, SpringState state, double target, double t) {
    const double y0 = state.value - target;
    const double v0 = state.velocity;
    const double zeta = spec.damping_ratio;
    const double w0 = std::sqrt(spec.stiffness);

    double y = 0.0;
    double v = 0.0;

    if (zeta < 1.0) {
        const double wd = w0 * std::sqrt(1.0 - zeta * zeta);
        const double a = y0;
        const double b = (v0 + zeta * w0 * y0) / wd;
        const double decay = std::exp(-zeta * w0 * t);
        const double c = std::cos(wd * t);
        const double s = std::sin(wd * t);
        y = decay * (a * c + b * s);
        v = decay * ((wd * b - zeta * w0 * a) * c - (wd * a + zeta * w0 * b) * s);
    } else if (zeta == 1.0) {
        const double a = y0;
        const double b = v0 + w0 * y0;
        const double decay = std::exp(-w0 * t);
        y = (a + b * t) * decay;
        v = (b - w0 * (a + b * t)) * decay;
    } else {
        const double root = w0 * std::sqrt(zeta * zeta - 1.0);
        const double r1 = -zeta * w0 + root;
        const double r2 = -zeta * w0 - root;
        const double c2 = (r1 * y0 - v0) / (r1 - r2);
        const double c1 = y0 - c2;
        const double e1 = std::exp(r1 * t);
        const double e2 = std::exp(r2 * t);
        y = c1 * e1 + c2 * e2;
        v = c1 * r1 * e1 + c2 * r2 * e2;
    }

    return {target + y, v};
}

bool is_settled(SpringState state, double target, SettleThreshold threshold) {
    return std::abs(state.value - target) < threshold.displacement && std::abs(state.velocity) < threshold.velocity;
}

}
