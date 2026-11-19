#include <doctest/doctest.h>

#include "lamina/tokens/motion.hpp"

using namespace lamina::tokens;

TEST_CASE("standard motion scheme matches the Compose tokens") {
    CHECK(standard_motion.spring(SpringKind::spatial, SpringSpeed::fast).damping_ratio == 0.9);
    CHECK(standard_motion.spring(SpringKind::spatial, SpringSpeed::fast).stiffness == 1400.0);
    CHECK(standard_motion.spring(SpringKind::spatial, SpringSpeed::standard).stiffness == 700.0);
    CHECK(standard_motion.spring(SpringKind::spatial, SpringSpeed::slow).stiffness == 300.0);
    CHECK(standard_motion.spring(SpringKind::effects, SpringSpeed::fast).stiffness == 3800.0);
    CHECK(standard_motion.spring(SpringKind::effects, SpringSpeed::standard).stiffness == 1600.0);
    CHECK(standard_motion.spring(SpringKind::effects, SpringSpeed::slow).stiffness == 800.0);
}

TEST_CASE("expressive motion scheme matches the Compose tokens") {
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::fast).damping_ratio == 0.6);
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::fast).stiffness == 800.0);
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::standard).damping_ratio == 0.8);
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::standard).stiffness == 380.0);
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::slow).damping_ratio == 0.8);
    CHECK(expressive_motion.spring(SpringKind::spatial, SpringSpeed::slow).stiffness == 200.0);
}

TEST_CASE("effects springs are shared between the schemes") {
    for (auto speed : {SpringSpeed::fast, SpringSpeed::standard, SpringSpeed::slow}) {
        const auto a = standard_motion.spring(SpringKind::effects, speed);
        const auto b = expressive_motion.spring(SpringKind::effects, speed);
        CHECK(a.damping_ratio == b.damping_ratio);
        CHECK(a.stiffness == b.stiffness);
    }
}

TEST_CASE("the widgets' motion scheme is the expressive one, and its role springs are its table entries") {
    CHECK(&motion_scheme == &expressive_motion);
    const auto same = [](lamina::motion::SpringSpec a, lamina::motion::SpringSpec b) {
        return a.damping_ratio == b.damping_ratio && a.stiffness == b.stiffness;
    };
    CHECK(same(motion_scheme.fast_spatial(), expressive_motion.spring(SpringKind::spatial, SpringSpeed::fast)));
    CHECK(same(motion_scheme.default_spatial(), expressive_motion.spring(SpringKind::spatial, SpringSpeed::standard)));
    CHECK(same(motion_scheme.slow_spatial(), expressive_motion.spring(SpringKind::spatial, SpringSpeed::slow)));
    CHECK(same(motion_scheme.fast_effects(), expressive_motion.spring(SpringKind::effects, SpringSpeed::fast)));
    CHECK(same(motion_scheme.default_effects(), expressive_motion.spring(SpringKind::effects, SpringSpeed::standard)));
    CHECK(same(motion_scheme.slow_effects(), expressive_motion.spring(SpringKind::effects, SpringSpeed::slow)));
}
