#include <doctest/doctest.h>

#include <cmath>
#include <utility>
#include <vector>

#include "cpp/cam/hct.h"
#include "lamina/color/scheme.hpp"

using namespace lamina::color;

namespace {

struct ExpectedTone {
    Role role;
    double light;
    double dark;
};

const std::vector<ExpectedTone> tonal_spot_tones{
    {Role::primary, 40, 80},
    {Role::on_primary, 100, 20},
    {Role::primary_container, 90, 30},
    {Role::on_primary_container, 30, 90},
    {Role::inverse_primary, 80, 40},
    {Role::secondary, 40, 80},
    {Role::on_secondary, 100, 20},
    {Role::secondary_container, 90, 30},
    {Role::on_secondary_container, 30, 90},
    {Role::tertiary, 40, 80},
    {Role::on_tertiary, 100, 20},
    {Role::tertiary_container, 90, 30},
    {Role::on_tertiary_container, 30, 90},
    {Role::error, 40, 80},
    {Role::on_error, 100, 20},
    {Role::error_container, 90, 30},
    {Role::on_error_container, 30, 90},
    {Role::background, 98, 6},
    {Role::on_background, 10, 90},
    {Role::surface, 98, 6},
    {Role::on_surface, 10, 90},
    {Role::surface_dim, 87, 6},
    {Role::surface_bright, 98, 24},
    {Role::surface_container_lowest, 100, 4},
    {Role::surface_container_low, 96, 10},
    {Role::surface_container, 94, 12},
    {Role::surface_container_high, 92, 17},
    {Role::surface_container_highest, 90, 22},
    {Role::surface_variant, 90, 30},
    {Role::on_surface_variant, 30, 80},
    {Role::inverse_surface, 20, 90},
    {Role::inverse_on_surface, 95, 20},
    {Role::outline, 50, 60},
    {Role::outline_variant, 80, 30},
    {Role::surface_tint, 40, 80},
    {Role::shadow, 0, 0},
    {Role::scrim, 0, 0},
    {Role::primary_fixed, 90, 90},
    {Role::primary_fixed_dim, 80, 80},
    {Role::on_primary_fixed, 10, 10},
    {Role::on_primary_fixed_variant, 30, 30},
    {Role::secondary_fixed, 90, 90},
    {Role::secondary_fixed_dim, 80, 80},
    {Role::on_secondary_fixed, 10, 10},
    {Role::on_secondary_fixed_variant, 30, 30},
    {Role::tertiary_fixed, 90, 90},
    {Role::tertiary_fixed_dim, 80, 80},
    {Role::on_tertiary_fixed, 10, 10},
    {Role::on_tertiary_fixed_variant, 30, 30},
};

constexpr double tone_tolerance = 0.6;

double tone_of(Argb argb) {
    return material_color_utilities::Hct(argb).get_tone();
}

}

TEST_CASE("every role has an expected tone in the test table") {
    CHECK(tonal_spot_tones.size() == role_count);
}

TEST_CASE("2021 TonalSpot roles land on the documented tones") {
    for (bool dark : {false, true}) {
        SchemeOptions options = default_scheme_options;
        options.dark = dark;
        const auto scheme = spec_2021().build(options);
        for (const auto& e : tonal_spot_tones) {
            CAPTURE(dark);
            CAPTURE(static_cast<int>(e.role));
            const double expected = dark ? e.dark : e.light;
            CHECK(std::abs(tone_of(scheme[e.role]) - expected) <= tone_tolerance);
        }
    }
}
