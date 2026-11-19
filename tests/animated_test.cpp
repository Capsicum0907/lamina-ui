#include <doctest/doctest.h>

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
