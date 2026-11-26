#include "lamina/motion/spline_decay.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::motion {

namespace {

double sign_of(double v) {
    return v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
}

std::vector<double> spline_positions(const SplineDecaySpec& s) {
    const double p1 = s.start_tension * s.inflection;
    const double p2 = 1.0 - s.end_tension * (1.0 - s.inflection);
    std::vector<double> positions(s.samples + 1, 0.0);
    double x_min = 0.0;
    for (std::size_t i = 0; i < s.samples; ++i) {
        const double alpha = static_cast<double>(i) / static_cast<double>(s.samples);
        double x_max = 1.0;
        double x = 0.0;
        double coef = 0.0;
        while (true) {
            x = x_min + (x_max - x_min) / 2.0;
            coef = 3.0 * x * (1.0 - x);
            const double tx = coef * ((1.0 - x) * p1 + x * p2) + x * x * x;
            if (std::abs(tx - alpha) < s.bisection_tolerance) {
                break;
            }
            if (tx > alpha) {
                x_max = x;
            } else {
                x_min = x;
            }
        }
        positions[i] = coef * ((1.0 - x) * s.start_tension + x) + x * x * x;
    }
    positions[s.samples] = 1.0;
    return positions;
}

}

SplineDecay::SplineDecay(SplineDecaySpec spec, double density)
    : spec_(spec),
      coefficient_(spec.friction * spec.gravity * spec.inches_per_meter * density * spec.dp_per_inch * spec.physical_friction),
      rate_(std::log(spec.deceleration_rate_base) / std::log(spec.deceleration_rate_reference)),
      positions_(spline_positions(spec)) {}

SplineDecay::Coefficients SplineDecay::at(double fraction) const {
    const double t = std::clamp(fraction, 0.0, 1.0);
    const double n = static_cast<double>(spec_.samples);
    const std::size_t index = static_cast<std::size_t>(n * t);
    if (index >= spec_.samples) {
        return {1.0, 0.0};
    }
    const double t_inf = static_cast<double>(index) / n;
    const double t_sup = static_cast<double>(index + 1) / n;
    const double d_inf = positions_[index];
    const double d_sup = positions_[index + 1];
    const double v = (d_sup - d_inf) / (t_sup - t_inf);
    return {d_inf + (t - t_inf) * v, v};
}

double SplineDecay::deceleration(double velocity) const {
    return std::log(spec_.inflection * std::abs(velocity) / coefficient_);
}

double SplineDecay::magnitude(double velocity) const {
    if (velocity == 0.0) {
        return 0.0;
    }
    return coefficient_ * std::exp(rate_ / (rate_ - 1.0) * deceleration(velocity));
}

double SplineDecay::distance(double velocity) const {
    return sign_of(velocity) * magnitude(velocity);
}

double SplineDecay::duration_seconds(double velocity) const {
    if (velocity == 0.0) {
        return 0.0;
    }
    return std::exp(deceleration(velocity) / (rate_ - 1.0));
}

double SplineDecay::position(double velocity, double time_seconds) const {
    const double duration = duration_seconds(velocity);
    const double fraction = duration > 0.0 ? time_seconds / duration : 1.0;
    return magnitude(velocity) * sign_of(velocity) * at(fraction).distance;
}

double SplineDecay::velocity(double velocity, double time_seconds) const {
    const double duration = duration_seconds(velocity);
    if (duration <= 0.0) {
        return 0.0;
    }
    return at(time_seconds / duration).velocity * sign_of(velocity) * magnitude(velocity) / duration;
}

void Fling::set_decay(const SplineDecay& decay) {
    if (&decay == decay_) {
        return;
    }
    const bool was_active = active_;
    const double v = velocity();
    decay_ = &decay;
    if (was_active) {
        start(value_, v);
    }
}

void Fling::start(double value, double velocity) {
    origin_ = value;
    value_ = value;
    initial_velocity_ = velocity;
    elapsed_ = 0.0;
    duration_ = decay_->duration_seconds(velocity);
    active_ = duration_ > 0.0;
}

double Fling::step(double dt) {
    if (!active_) {
        return value_;
    }
    elapsed_ += dt;
    value_ = origin_ + decay_->position(initial_velocity_, elapsed_);
    if (elapsed_ >= duration_) {
        active_ = false;
    }
    return value_;
}

double Fling::velocity() const {
    return active_ ? decay_->velocity(initial_velocity_, elapsed_) : 0.0;
}

}
