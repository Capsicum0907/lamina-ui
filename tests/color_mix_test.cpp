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

TEST_CASE("alpha reads the top byte") {
    CHECK(color::alpha(0x80123456) == 0x80);
    CHECK(color::alpha(0x00ffffff) == 0);
}

TEST_CASE("scale_alpha multiplies the existing alpha and keeps the color") {
    CHECK(color::scale_alpha(0xff123456, 0.5) == 0x80123456);
    CHECK(color::scale_alpha(0x80123456, 0.5) == 0x40123456);
    CHECK(color::scale_alpha(0x80123456, 1.0) == 0x80123456);
    CHECK(color::scale_alpha(0xff123456, 0.0) == 0x00123456);
}
