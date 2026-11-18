#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class ButtonSize : std::size_t { extra_small, small, medium, large, extra_large, count };
enum class ButtonStyle : std::size_t { filled, tonal, outlined, text, elevated, count };
enum class ButtonShape { round, square };

struct ButtonSizeTokens {
    double height;
    CornerSize square_corner;
    CornerSize pressed_corner;
    double icon_size;
    double icon_label_space;
    double leading_space;
    double trailing_space;
    double outline_width;
    TypeRole label;
};

inline constexpr std::array<ButtonSizeTokens, static_cast<std::size_t>(ButtonSize::count)> button_sizes{{
    {32.0, CornerSize::medium, CornerSize::small, 20.0, 8.0, 12.0, 12.0, 1.0, TypeRole::label_large},
    {40.0, CornerSize::medium, CornerSize::small, 20.0, 8.0, 16.0, 16.0, 1.0, TypeRole::label_large},
    {56.0, CornerSize::large, CornerSize::medium, 24.0, 8.0, 24.0, 24.0, 1.0, TypeRole::title_medium},
    {96.0, CornerSize::extra_large, CornerSize::large, 32.0, 12.0, 48.0, 48.0, 2.0, TypeRole::headline_small},
    {136.0, CornerSize::extra_large, CornerSize::large, 40.0, 16.0, 64.0, 64.0, 3.0, TypeRole::headline_large},
}};

inline constexpr double button_min_width = 58.0;

struct ButtonStyleTokens {
    std::optional<color::Role> container;
    color::Role label;
    std::optional<color::Role> outline;
    std::size_t elevation;
    std::size_t hovered_elevation;
    std::size_t pressed_elevation;
};

inline constexpr std::array<ButtonStyleTokens, static_cast<std::size_t>(ButtonStyle::count)> button_styles{{
    {color::Role::primary, color::Role::on_primary, std::nullopt, 0, 1, 0},
    {color::Role::secondary_container, color::Role::on_secondary_container, std::nullopt, 0, 1, 0},
    {std::nullopt, color::Role::on_surface_variant, color::Role::outline_variant, 0, 0, 0},
    {std::nullopt, color::Role::primary, std::nullopt, 0, 0, 0},
    {color::Role::surface_container_low, color::Role::primary, std::nullopt, 1, 2, 1},
}};

constexpr const ButtonSizeTokens& button_size(ButtonSize s) {
    return button_sizes[static_cast<std::size_t>(s)];
}

constexpr const ButtonStyleTokens& button_style(ButtonStyle s) {
    return button_styles[static_cast<std::size_t>(s)];
}

}
