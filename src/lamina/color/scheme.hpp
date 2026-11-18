#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace lamina::color {

using Argb = std::uint32_t;

enum class Role : std::size_t {
    background,
    on_background,
    surface,
    surface_dim,
    surface_bright,
    surface_container_lowest,
    surface_container_low,
    surface_container,
    surface_container_high,
    surface_container_highest,
    on_surface,
    surface_variant,
    on_surface_variant,
    inverse_surface,
    inverse_on_surface,
    outline,
    outline_variant,
    shadow,
    scrim,
    surface_tint,
    primary,
    on_primary,
    primary_container,
    on_primary_container,
    inverse_primary,
    secondary,
    on_secondary,
    secondary_container,
    on_secondary_container,
    tertiary,
    on_tertiary,
    tertiary_container,
    on_tertiary_container,
    error,
    on_error,
    error_container,
    on_error_container,
    primary_fixed,
    primary_fixed_dim,
    on_primary_fixed,
    on_primary_fixed_variant,
    secondary_fixed,
    secondary_fixed_dim,
    on_secondary_fixed,
    on_secondary_fixed_variant,
    tertiary_fixed,
    tertiary_fixed_dim,
    on_tertiary_fixed,
    on_tertiary_fixed_variant,
    count,
};

inline constexpr std::size_t role_count = static_cast<std::size_t>(Role::count);

enum class Variant {
    monochrome,
    neutral,
    tonal_spot,
    vibrant,
    expressive,
    fidelity,
    content,
    rainbow,
    fruit_salad,
};

struct SchemeOptions {
    Argb seed;
    bool dark;
    Variant variant;
    double contrast;
};

inline constexpr SchemeOptions default_scheme_options{
    .seed = 0xff6750a4,
    .dark = false,
    .variant = Variant::tonal_spot,
    .contrast = 0.0,
};

struct ColorScheme {
    std::array<Argb, role_count> colors;

    constexpr Argb operator[](Role role) const { return colors[static_cast<std::size_t>(role)]; }
};

class ColorSpec {
public:
    virtual ~ColorSpec() = default;
    virtual ColorScheme build(const SchemeOptions& options) const = 0;
};

const ColorSpec& spec_2021();

}
