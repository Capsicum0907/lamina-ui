#include <doctest/doctest.h>

#include <cstddef>

#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/surface.hpp"

using namespace lamina;

namespace {

struct Levels {
    std::size_t elevation = 1;
    std::size_t hovered_elevation = 2;
    std::size_t pressed_elevation = 3;
};

}

TEST_CASE("state_elevation picks the pressed, then the hovered, then the resting level") {
    const Levels levels;
    widgets::PressState press;
    CHECK(widgets::state_elevation(levels, press) == tokens::elevation_dp[1]);
    press.hovered = true;
    CHECK(widgets::state_elevation(levels, press) == tokens::elevation_dp[2]);
    press.pressed = true;
    CHECK(widgets::state_elevation(levels, press) == tokens::elevation_dp[3]);
    press.hovered = false;
    CHECK(widgets::state_elevation(levels, press) == tokens::elevation_dp[3]);
}
