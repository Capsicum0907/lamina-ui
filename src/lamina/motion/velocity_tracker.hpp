#pragma once

#include <cstddef>
#include <vector>

namespace lamina::motion {

struct VelocityTrackerSpec {
    std::size_t history;
    double horizon_seconds;
    double stopped_seconds;
    std::size_t min_samples;
    std::size_t degree;
};

class VelocityTracker {
public:
    explicit VelocityTracker(VelocityTrackerSpec spec);

    void add(double time_seconds, double position);
    void release(double time_seconds);
    void reset();
    double velocity() const;
    double velocity(double maximum) const;
    std::size_t samples() const { return count_; }

private:
    struct Sample {
        double time;
        double position;
    };

    VelocityTrackerSpec spec_;
    std::vector<Sample> ring_;
    std::size_t newest_ = 0;
    std::size_t count_ = 0;
};

double least_squares_slope(const std::vector<double>& x, const std::vector<double>& y, std::size_t degree);

}
