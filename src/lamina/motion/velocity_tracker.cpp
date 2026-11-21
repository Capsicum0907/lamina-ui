#include "lamina/motion/velocity_tracker.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::motion {

namespace {

constexpr double milliseconds_per_second = 1000.0;
constexpr double minimum_norm = 1e-6;

double dot(const std::vector<double>& a, const std::vector<double>& b) {
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

}

VelocityTracker::VelocityTracker(VelocityTrackerSpec spec) : spec_(spec), ring_(spec.history) {}

void VelocityTracker::add(double time_seconds, double position) {
    newest_ = count_ == 0 ? 0 : (newest_ + 1) % ring_.size();
    ring_[newest_] = {time_seconds, position};
    count_ = std::min(count_ + 1, ring_.size());
}

void VelocityTracker::release(double time_seconds) {
    if (count_ > 0 && time_seconds - ring_[newest_].time > spec_.stopped_seconds) {
        reset();
    }
}

void VelocityTracker::reset() {
    count_ = 0;
    newest_ = 0;
}

double VelocityTracker::velocity() const {
    if (count_ == 0) {
        return 0.0;
    }
    std::vector<double> times;
    std::vector<double> positions;
    const Sample newest = ring_[newest_];
    Sample previous = newest;
    std::size_t index = newest_;
    for (std::size_t k = 0; k < count_; ++k) {
        const Sample s = ring_[index];
        const double age = newest.time - s.time;
        const double gap = std::abs(s.time - previous.time);
        previous = s;
        if (age > spec_.horizon_seconds || gap > spec_.stopped_seconds) {
            break;
        }
        positions.push_back(s.position);
        times.push_back(-age * milliseconds_per_second);
        index = (index == 0 ? ring_.size() : index) - 1;
    }
    if (positions.size() < spec_.min_samples) {
        return 0.0;
    }
    return least_squares_slope(times, positions, spec_.degree) * milliseconds_per_second;
}

double VelocityTracker::velocity(double maximum) const {
    return std::clamp(velocity(), -maximum, maximum);
}

double least_squares_slope(const std::vector<double>& x, const std::vector<double>& y, std::size_t degree) {
    const std::size_t m = x.size();
    if (m == 0 || degree == 0) {
        return 0.0;
    }
    const std::size_t n = std::min(degree, m - 1) + 1;
    if (n < 2) {
        return 0.0;
    }
    std::vector<std::vector<double>> a(n, std::vector<double>(m));
    for (std::size_t h = 0; h < m; ++h) {
        a[0][h] = 1.0;
        for (std::size_t i = 1; i < n; ++i) {
            a[i][h] = a[i - 1][h] * x[h];
        }
    }
    std::vector<std::vector<double>> q(n, std::vector<double>(m));
    std::vector<std::vector<double>> r(n, std::vector<double>(n, 0.0));
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<double>& w = q[j];
        w = a[j];
        for (std::size_t i = 0; i < j; ++i) {
            const double d = dot(w, q[i]);
            for (std::size_t h = 0; h < m; ++h) {
                w[h] -= d * q[i][h];
            }
        }
        const double inverse_norm = 1.0 / std::max(std::sqrt(dot(w, w)), minimum_norm);
        for (double& v : w) {
            v *= inverse_norm;
        }
        for (std::size_t i = j; i < n; ++i) {
            r[j][i] = dot(w, a[i]);
        }
    }
    std::vector<double> coefficients(n, 0.0);
    for (std::size_t i = n; i-- > 0;) {
        double c = dot(q[i], y);
        for (std::size_t j = n - 1; j > i; --j) {
            c -= r[i][j] * coefficients[j];
        }
        coefficients[i] = c / r[i][i];
    }
    return coefficients[1];
}

}
