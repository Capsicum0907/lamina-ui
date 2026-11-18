#include <doctest/doctest.h>

#include <algorithm>
#include <array>
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
    {Role::primary_dim, 40, 80},
    {Role::secondary_dim, 40, 80},
    {Role::tertiary_dim, 40, 80},
    {Role::error_dim, 40, 80},
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

namespace {

struct ReferenceColor {
    SchemeOptions options;
    Role role;
    Argb argb;
};

constexpr Argb yellow_seed = 0xffcddc39;
constexpr Argb teal_seed = 0xff009688;

constexpr SchemeOptions options_for(Argb seed, Variant variant, bool dark, double contrast) {
    return {.seed = seed, .dark = dark, .variant = variant, .contrast = contrast};
}

const std::vector<ReferenceColor> java_2025_reference{
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::surface, 0xfffdf7fe},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::surface_container, 0xfff2ecf5},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::on_surface, 0xff34313a},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::outline, 0xff7d7983},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::primary, 0xff655789},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::on_primary, 0xfffdf7ff},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::primary_container, 0xffd4c3fd},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::primary_fixed_dim, 0xffc6b6ee},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::primary_dim, 0xff594b7c},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::secondary_dim, 0xff565065},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::tertiary_dim, 0xff6e4664},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0), Role::error_dim, 0xff6b0221},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::surface, 0xff0f0d12},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::on_surface, 0xffeae3ef},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::primary, 0xffcdc0ec},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::primary_container, 0xff574d72},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::primary_dim, 0xffbfb2de},
    {options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0), Role::error_dim, 0xffc44b5f},
    {options_for(yellow_seed, Variant::expressive, false, 0.0), Role::surface, 0xfffdfee9},
    {options_for(yellow_seed, Variant::expressive, false, 0.0), Role::surface_dim, 0xffdfeb74},
    {options_for(yellow_seed, Variant::expressive, false, 0.0), Role::primary, 0xff393f00},
    {options_for(yellow_seed, Variant::expressive, false, 0.0), Role::tertiary_container, 0xffa5b5ff},
    {options_for(yellow_seed, Variant::expressive, true, 0.0), Role::surface, 0xff0e0f08},
    {options_for(yellow_seed, Variant::expressive, true, 0.0), Role::primary, 0xffeff7a0},
    {options_for(default_scheme_options.seed, Variant::neutral, false, 1.0), Role::primary, 0xff25232b},
    {options_for(default_scheme_options.seed, Variant::neutral, false, 1.0), Role::on_surface_variant, 0xff252326},
    {options_for(default_scheme_options.seed, Variant::vibrant, false, 1.0), Role::primary, 0xff270067},
    {options_for(default_scheme_options.seed, Variant::vibrant, false, 1.0), Role::on_surface_variant, 0xff28173c},
    {options_for(teal_seed, Variant::fidelity, true, 0.0), Role::primary, 0xff67d9c9},
    {options_for(teal_seed, Variant::fidelity, true, 0.0), Role::primary_dim, 0xff85f6e5},
};

constexpr std::array<Variant, 9> all_variants{
    Variant::monochrome, Variant::neutral, Variant::tonal_spot, Variant::vibrant, Variant::expressive,
    Variant::fidelity, Variant::content, Variant::rainbow, Variant::fruit_salad,
};

constexpr std::array<double, 4> contrast_levels{-1.0, 0.0, 0.5, 1.0};

constexpr std::array<Variant, 5> variants_without_2025_rules{
    Variant::monochrome, Variant::fidelity, Variant::content, Variant::rainbow, Variant::fruit_salad,
};

constexpr std::array<std::pair<Role, Role>, 4> dim_roles{{
    {Role::primary_dim, Role::primary},
    {Role::secondary_dim, Role::secondary},
    {Role::tertiary_dim, Role::tertiary},
    {Role::error_dim, Role::error},
}};

constexpr Argb opaque = 0xff000000;

}

TEST_CASE("2025 colors match the Java material-color-utilities at sampled values") {
    for (const auto& e : java_2025_reference) {
        CAPTURE(e.options.seed);
        CAPTURE(static_cast<int>(e.options.variant));
        CAPTURE(e.options.dark);
        CAPTURE(e.options.contrast);
        CAPTURE(static_cast<int>(e.role));
        CHECK(spec_2025().build(e.options)[e.role] == e.argb);
    }
}

TEST_CASE("2025 fills every role with an opaque color for every variant, mode and contrast") {
    for (Variant variant : all_variants) {
        for (bool dark : {false, true}) {
            for (double contrast : contrast_levels) {
                const auto scheme = spec_2025().build(options_for(default_scheme_options.seed, variant, dark, contrast));
                for (std::size_t i = 0; i < role_count; ++i) {
                    CAPTURE(static_cast<int>(variant));
                    CAPTURE(dark);
                    CAPTURE(contrast);
                    CAPTURE(i);
                    CHECK((scheme.colors[i] & opaque) == opaque);
                }
            }
        }
    }
}

TEST_CASE("2025 TonalSpot surfaces are light in light mode and dark in dark mode") {
    const auto light = spec_2025().build(options_for(default_scheme_options.seed, Variant::tonal_spot, false, 0.0));
    const auto dark = spec_2025().build(options_for(default_scheme_options.seed, Variant::tonal_spot, true, 0.0));
    CHECK(std::abs(tone_of(light[Role::surface]) - 98.0) <= tone_tolerance);
    CHECK(std::abs(tone_of(dark[Role::surface]) - 4.0) <= tone_tolerance);
    CHECK(tone_of(light[Role::on_surface]) < tone_of(light[Role::surface]));
    CHECK(tone_of(dark[Role::on_surface]) > tone_of(dark[Role::surface]));
    CHECK(tone_of(light[Role::primary_dim]) < tone_of(light[Role::primary]));
    CHECK(tone_of(dark[Role::primary_dim]) < tone_of(dark[Role::primary]));
}

TEST_CASE("2025 keeps the 2021 colors for variants it has no rules for") {
    for (Variant variant : variants_without_2025_rules) {
        for (bool dark : {false, true}) {
            const auto options = options_for(teal_seed, variant, dark, 0.0);
            const auto a = spec_2021().build(options);
            const auto b = spec_2025().build(options);
            for (std::size_t i = 0; i < role_count; ++i) {
                const auto role = static_cast<Role>(i);
                if (std::ranges::any_of(dim_roles, [role](const auto& d) { return d.first == role; })) {
                    continue;
                }
                CAPTURE(static_cast<int>(variant));
                CAPTURE(i);
                CHECK(a[role] == b[role]);
            }
        }
    }
}

TEST_CASE("2021 gives each dim role the color of its base role") {
    for (bool dark : {false, true}) {
        SchemeOptions options = default_scheme_options;
        options.dark = dark;
        const auto scheme = spec_2021().build(options);
        for (const auto& [dim, base] : dim_roles) {
            CHECK(scheme[dim] == scheme[base]);
        }
    }
}
