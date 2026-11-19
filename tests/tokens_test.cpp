#include <doctest/doctest.h>

#include <array>

#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

using namespace lamina::tokens;

TEST_CASE("corner radii follow the shape scale") {
    CHECK(corner_radius(CornerSize::none, 100, 40) == 0.0);
    CHECK(corner_radius(CornerSize::medium, 100, 40) == 12.0);
    CHECK(corner_radius(CornerSize::extra_large, 100, 40) == 28.0);
    CHECK(corner_radius(CornerSize::extra_extra_large, 100, 40) == 48.0);
}

TEST_CASE("full corners are half of the shorter side") {
    CHECK(corner_radius(CornerSize::full, 100, 40) == 20.0);
    CHECK(corner_radius(CornerSize::full, 30, 90) == 15.0);
}

TEST_CASE("corner radii increase step by step") {
    for (std::size_t i = 1; i < corner_radius_dp.size(); ++i) {
        CHECK(corner_radius_dp[i] > corner_radius_dp[i - 1]);
    }
}

namespace {

struct WebTypeStyle {
    double size_rem;
    double line_height_rem;
    double tracking_rem;
};

constexpr double px_per_rem = 16.0;

constexpr std::array<WebTypeStyle, type_role_count> material_web_type_scale{{
    {3.5625, 4.0, -0.015625},
    {2.8125, 3.25, 0.0},
    {2.25, 2.75, 0.0},
    {2.0, 2.5, 0.0},
    {1.75, 2.25, 0.0},
    {1.5, 2.0, 0.0},
    {1.375, 1.75, 0.0},
    {1.0, 1.5, 0.009375},
    {0.875, 1.25, 0.00625},
    {1.0, 1.5, 0.03125},
    {0.875, 1.25, 0.015625},
    {0.75, 1.0, 0.025},
    {0.875, 1.25, 0.00625},
    {0.75, 1.0, 0.03125},
    {0.6875, 1.0, 0.03125},
}};

}

TEST_CASE("type scale matches the material-web tokens") {
    for (std::size_t i = 0; i < type_role_count; ++i) {
        const auto& web = material_web_type_scale[i];
        const auto style = type_style(static_cast<TypeRole>(i));
        CAPTURE(i);
        CHECK(style.size_sp == doctest::Approx(web.size_rem * px_per_rem));
        CHECK(style.line_height_sp == doctest::Approx(web.line_height_rem * px_per_rem));
        CHECK(style.tracking_sp == doctest::Approx(web.tracking_rem * px_per_rem));
    }
}

TEST_CASE("emphasized styles change only the weight") {
    for (std::size_t i = 0; i < type_role_count; ++i) {
        const auto role = static_cast<TypeRole>(i);
        const auto plain = type_style(role);
        const auto strong = type_style(role, true);
        CAPTURE(i);
        CHECK(strong.size_sp == plain.size_sp);
        CHECK(strong.line_height_sp == plain.line_height_sp);
        CHECK(strong.tracking_sp == plain.tracking_sp);
        CHECK(strong.weight > plain.weight);
    }
}

TEST_CASE("elevation and state layer tokens") {
    CHECK(elevation_dp == std::array<double, 6>{0, 1, 3, 6, 8, 12});
    CHECK(state_layer_opacity::hover < state_layer_opacity::pressed);
    CHECK(state_layer_opacity::dragged == 0.16);
}

TEST_CASE("elevation shadows match the material-web levels") {
    struct Expected {
        double key_y, key_blur, ambient_y, ambient_blur, ambient_spread;
    };
    const Expected levels[] = {
        {0, 0, 0, 0, 0}, {1, 2, 1, 3, 1}, {1, 2, 2, 6, 2}, {1, 3, 4, 8, 3}, {2, 3, 6, 10, 4}, {4, 4, 8, 12, 6},
    };
    for (int level = 0; level <= 5; ++level) {
        const auto key = key_shadow(level);
        const auto ambient = ambient_shadow(level);
        const auto& e = levels[level];
        CAPTURE(level);
        CHECK(key.offset_y == e.key_y);
        CHECK(key.blur == e.key_blur);
        CHECK(ambient.offset_y == e.ambient_y);
        CHECK(ambient.blur == e.ambient_blur);
        CHECK(ambient.spread == e.ambient_spread);
        CHECK(level_for_elevation(elevation_dp[level]) == doctest::Approx(level));
    }
}
