#include <doctest/doctest.h>

#include "lamina/color/mix.hpp"

using namespace lamina;

TEST_CASE("mix interpolates every channel") {
    CHECK(color::mix(0xff000000, 0xffffffff, 0.0) == 0xff000000);
    CHECK(color::mix(0xff000000, 0xffffffff, 1.0) == 0xffffffff);
    CHECK(color::mix(0xff000000, 0xff0000ff, 0.5) == 0xff000080);
    CHECK(color::mix(0xff000000, 0xffffffff, 2.0) == 0xffffffff);
}

TEST_CASE("state_layer composites the layer over the container") {
    CHECK(color::state_layer(0xffffffff, 0x00000000, 0.4) == 0xff999999);
    CHECK(color::state_layer(0x00000000, 0xff123456, 0.4) == color::with_alpha(0xff123456, 0.4));
    CHECK(color::source_over(0x80ff0000, 0x800000ff) == 0xc0aa0055);
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

TEST_CASE("faded scales the alpha of every role and keeps the colors") {
    color::ColorScheme scheme{};
    for (std::size_t i = 0; i < color::role_count; ++i) {
        scheme.colors[i] = 0xff000000 | static_cast<color::Argb>(i * 0x010203);
    }
    const color::ColorScheme half = color::faded(scheme, 0.5);
    for (std::size_t i = 0; i < color::role_count; ++i) {
        CHECK(half.colors[i] == color::scale_alpha(scheme.colors[i], 0.5));
    }
    CHECK(color::mix(half.colors[1], half.colors[2], 0.3) == color::scale_alpha(color::mix(scheme.colors[1], scheme.colors[2], 0.3), 0.5));
}
