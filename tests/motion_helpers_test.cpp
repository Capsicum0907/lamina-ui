#include <doctest/doctest.h>

#include "lamina/color/mix.hpp"
#include "lamina/motion/animated.hpp"
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

TEST_CASE("mix interpolates every channel") {
    CHECK(color::mix(0xff000000, 0xffffffff, 0.0) == 0xff000000);
    CHECK(color::mix(0xff000000, 0xffffffff, 1.0) == 0xffffffff);
    CHECK(color::mix(0xff000000, 0xff0000ff, 0.5) == 0xff000080);
    CHECK(color::mix(0xff000000, 0xffffffff, 2.0) == 0xffffffff);
}

TEST_CASE("overlay keeps the base alpha") {
    CHECK(color::overlay(0xffffffff, 0x00000000, 0.5) == 0xff808080);
    CHECK(color::with_alpha(0xff123456, 0.0) == 0x00123456);
}
