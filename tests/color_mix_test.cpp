#include <doctest/doctest.h>

#include "lamina/color/mix.hpp"

using namespace lamina;

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
