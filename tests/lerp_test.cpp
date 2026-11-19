#include <doctest/doctest.h>

#include "lamina/motion/lerp.hpp"

using namespace lamina;

TEST_CASE("lerp moves from a to b as t goes from 0 to 1 and extrapolates beyond") {
    CHECK(motion::lerp(2.0, 6.0, 0.0) == 2.0);
    CHECK(motion::lerp(2.0, 6.0, 0.25) == 3.0);
    CHECK(motion::lerp(2.0, 6.0, 1.0) == 6.0);
    CHECK(motion::lerp(2.0, 6.0, 1.5) == 8.0);
}
