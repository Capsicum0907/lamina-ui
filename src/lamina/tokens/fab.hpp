#pragma once

#include <array>
#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/shape.hpp"

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

constexpr const FabSizeTokens& fab_size(FabSize s) {
    return fab_sizes[static_cast<std::size_t>(s)];
}

constexpr const FabStyleTokens& fab_style(FabStyle s) {
    return fab_styles[static_cast<std::size_t>(s)];
}

}
