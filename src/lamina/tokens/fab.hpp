#pragma once

#include <array>
#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class FabSize : std::size_t { baseline, medium, large, count };
enum class FabStyle : std::size_t { primary_container, secondary_container, count };

struct FabSizeTokens {
    double size;
    CornerSize corner;
    double icon_size;
};

inline constexpr std::array<FabSizeTokens, static_cast<std::size_t>(FabSize::count)> fab_sizes{{
    {56.0, CornerSize::large, 24.0},
    {80.0, CornerSize::large_increased, 28.0},
    {96.0, CornerSize::extra_large, 36.0},
}};

struct FabStyleTokens {
    color::Role container;
    color::Role content;
    std::size_t elevation;
    std::size_t hovered_elevation;
    std::size_t pressed_elevation;
};

inline constexpr std::array<FabStyleTokens, static_cast<std::size_t>(FabStyle::count)> fab_styles{{
    {color::Role::primary_container, color::Role::on_primary_container, 3, 4, 3},
    {color::Role::secondary_container, color::Role::on_secondary_container, 3, 4, 3},
}};

enum class ExtendedFabSize : std::size_t { small, medium, large, count };

struct ExtendedFabSizeTokens {
    double height;
    CornerSize corner;
    double icon_size;
    double leading_space;
    double trailing_space;
    double icon_label_space;
    TypeRole label;
};

inline constexpr std::array<ExtendedFabSizeTokens, static_cast<std::size_t>(ExtendedFabSize::count)> extended_fab_sizes{{
    {56.0, CornerSize::large, 24.0, 16.0, 16.0, 8.0, TypeRole::title_medium},
    {80.0, CornerSize::large_increased, 28.0, 26.0, 26.0, 12.0, TypeRole::title_large},
    {96.0, CornerSize::extra_large, 32.0, 28.0, 28.0, 16.0, TypeRole::headline_small},
}};

struct ExtendedFabMotion {
    motion::SpringSpec width;
    motion::SpringSpec label_alpha;
};

inline constexpr ExtendedFabMotion extended_fab_motion{
    .width = motion_scheme.fast_spatial(),
    .label_alpha = motion_scheme.fast_effects(),
};

constexpr const ExtendedFabSizeTokens& extended_fab_size(ExtendedFabSize s) {
    return extended_fab_sizes[static_cast<std::size_t>(s)];
}

constexpr const FabSizeTokens& fab_size(FabSize s) {
    return fab_sizes[static_cast<std::size_t>(s)];
}

constexpr const FabStyleTokens& fab_style(FabStyle s) {
    return fab_styles[static_cast<std::size_t>(s)];
}

}
