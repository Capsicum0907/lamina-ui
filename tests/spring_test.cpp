#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "lamina/motion/spring.hpp"
#include "lamina/tokens/motion.hpp"

using namespace lamina;
using motion::SpringSpec;
using motion::SpringState;

namespace {

SpringState integrate_rk4(const SpringSpec& spec, SpringState s, double target, double duration, int steps) {
    const double h = duration / steps;
    const double c = 2.0 * spec.damping_ratio * std::sqrt(spec.stiffness);
    auto accel = [&](double x, double v) { return -spec.stiffness * (x - target) - c * v; };
    for (int i = 0; i < steps; ++i) {
        const double k1x = s.velocity;
        const double k1v = accel(s.value, s.velocity);
        const double k2x = s.velocity + 0.5 * h * k1v;
        const double k2v = accel(s.value + 0.5 * h * k1x, s.velocity + 0.5 * h * k1v);
        const double k3x = s.velocity + 0.5 * h * k2v;
        const double k3v = accel(s.value + 0.5 * h * k2x, s.velocity + 0.5 * h * k2v);
        const double k4x = s.velocity + h * k3v;
        const double k4v = accel(s.value + h * k3x, s.velocity + h * k3v);
        s.value += h / 6.0 * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
        s.velocity += h / 6.0 * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
    }
    return s;
}

}

TEST_CASE("closed form agrees with numerical integration in every damping regime") {
    const SpringState start{0.0, 3.0};
    const double target = 1.0;
    for (double zeta : {0.3, 0.6, 1.0, 1.7}) {
        const SpringSpec spec{zeta, 500.0};
        for (double t : {0.01, 0.05, 0.2, 0.6}) {
            const auto exact = motion::advance(spec, start, target, t);
            const auto numeric = integrate_rk4(spec, start, target, t, 20000);
            CAPTURE(zeta);
            CAPTURE(t);
            CHECK(exact.value == doctest::Approx(numeric.value).epsilon(1e-9));
            CHECK(exact.velocity == doctest::Approx(numeric.velocity).epsilon(1e-9));
        }
    }
}

TEST_CASE("advancing in pieces equals advancing at once") {
    const SpringSpec spec = tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast);
    const SpringState start{0.0, 0.0};
    const auto once = motion::advance(spec, start, 1.0, 0.1);
    SpringState pieces = start;
    for (int i = 0; i < 6; ++i) {
        pieces = motion::advance(spec, pieces, 1.0, 0.1 / 6.0);
    }
    CHECK(pieces.value == doctest::Approx(once.value).epsilon(1e-12));
    CHECK(pieces.velocity == doctest::Approx(once.velocity).epsilon(1e-12));
}

TEST_CASE("zero time leaves the state unchanged") {
    const SpringSpec spec{0.8, 380.0};
    const auto s = motion::advance(spec, {0.25, -2.0}, 1.0, 0.0);
    CHECK(s.value == doctest::Approx(0.25));
    CHECK(s.velocity == doctest::Approx(-2.0));
}

namespace {

double peak(const SpringSpec& spec) {
    double highest = 0.0;
    for (int i = 1; i <= 2000; ++i) {
        highest = std::max(highest, motion::advance(spec, {0.0, 0.0}, 1.0, i * 0.001).value);
    }
    return highest;
}

}

TEST_CASE("underdamped springs overshoot and critically damped ones do not") {
    using enum tokens::SpringSpeed;
    const auto expressive_fast = tokens::expressive_motion.spring(tokens::SpringKind::spatial, fast);
    const double zeta = expressive_fast.damping_ratio;
    const double expected_overshoot = std::exp(-zeta * 3.14159265358979323846 / std::sqrt(1.0 - zeta * zeta));
    CHECK(peak(expressive_fast) - 1.0 == doctest::Approx(expected_overshoot).epsilon(1e-3));

    const auto effects = tokens::expressive_motion.spring(tokens::SpringKind::effects, standard);
    CHECK(peak(effects) <= 1.0);
}

TEST_CASE("every token spring settles within two seconds") {
    using tokens::SpringKind;
    using tokens::SpringSpeed;
    for (const auto* scheme : {&tokens::standard_motion, &tokens::expressive_motion}) {
        for (auto kind : {SpringKind::spatial, SpringKind::effects}) {
            for (auto speed : {SpringSpeed::fast, SpringSpeed::standard, SpringSpeed::slow}) {
                const auto s = motion::advance(scheme->spring(kind, speed), {0.0, 0.0}, 1.0, 2.0);
                CHECK(motion::is_settled(s, 1.0));
            }
        }
    }
}
