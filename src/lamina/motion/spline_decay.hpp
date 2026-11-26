#pragma once

#include <cstddef>
#include <vector>

namespace lamina::motion {

struct SplineDecaySpec {
    double friction;
    double inflection;
    double start_tension;
    double end_tension;
    std::size_t samples;
    double bisection_tolerance;
    double gravity;
    double inches_per_meter;
    double dp_per_inch;
    double physical_friction;
    double deceleration_rate_base;
    double deceleration_rate_reference;

    bool operator==(const SplineDecaySpec&) const = default;
};

class SplineDecay {
public:
    explicit SplineDecay(SplineDecaySpec spec, double density = 1.0);

    double distance(double velocity) const;
    double duration_seconds(double velocity) const;
    double position(double velocity, double time_seconds) const;
    double velocity(double velocity, double time_seconds) const;

private:
    struct Coefficients {
        double distance;
        double velocity;
    };

    Coefficients at(double fraction) const;
    double deceleration(double velocity) const;
    double magnitude(double velocity) const;

    SplineDecaySpec spec_;
    double coefficient_;
    double rate_;
    std::vector<double> positions_;
};

class Fling {
public:
    explicit Fling(const SplineDecay& decay) : decay_(&decay) {}

    void set_decay(const SplineDecay& decay);

    void start(double value, double velocity);
    void stop() { active_ = false; }
    bool active() const { return active_; }
    double step(double dt_seconds);
    double value() const { return value_; }
    double velocity() const;

private:
    const SplineDecay* decay_;
    bool active_ = false;
    double origin_ = 0.0;
    double initial_velocity_ = 0.0;
    double elapsed_ = 0.0;
    double duration_ = 0.0;
    double value_ = 0.0;
};

}
