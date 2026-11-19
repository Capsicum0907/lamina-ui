#include <doctest/doctest.h>

#include <algorithm>

#include "lamina/motion/animated.hpp"
#include "lamina/motion/animated_radii.hpp"
#include "lamina/tokens/motion.hpp"

using namespace lamina;

TEST_CASE("an animated value reaches its target and snaps when settled") {
    motion::Animated a(0.0, tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::fast));
    a.animate_to(1.0);
    for (int i = 0; i < 120; ++i) {
        a.step(1.0 / 60.0);
    }
    CHECK(a.settled());
    a.step(1.0 / 60.0);
    CHECK(a.value() == 1.0);
}

TEST_CASE("animated radii move each corner on its spring and never go below zero") {
    motion::AnimatedRadii r(render::uniform_radii(16.0), tokens::motion_scheme.fast_spatial());
    CHECK(r.settled());
    r.animate_to({0.0, 8.0, 16.0, 24.0});
    CHECK(!r.settled());
    double lowest = 16.0;
    for (int i = 0; i < 240; ++i) {
        r.step(1.0 / 240.0);
        lowest = std::min(lowest, r.value().top_left);
    }
    CHECK(lowest == 0.0);
    for (int i = 0; i < 480 && !r.settled(); ++i) {
        r.step(1.0 / 60.0);
    }
    CHECK(r.settled());
    CHECK(r.value().top_right == doctest::Approx(8.0));
    CHECK(r.value().bottom_left == doctest::Approx(24.0));
    r.snap_to(render::uniform_radii(4.0));
    CHECK(r.settled());
    CHECK(r.value().bottom_right == 4.0);
}
