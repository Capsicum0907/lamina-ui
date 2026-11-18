#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

enum class IconButtonSize : std::size_t { extra_small, small, medium, large, extra_large, count };
enum class IconButtonStyle : std::size_t { filled, tonal, outlined, standard, count };
enum class IconButtonWidth : std::size_t { narrow, uniform, wide, count };

struct IconButtonSizeTokens {
    double height;
    double icon_size;
    std::array<double, static_cast<std::size_t>(IconButtonWidth::count)> side_space;
    CornerSize square_corner;
    CornerSize pressed_corner;
    CornerSize selected_round_corner;
    CornerSize selected_square_corner;
    double outline_width;
};

inline constexpr std::array<IconButtonSizeTokens, static_cast<std::size_t>(IconButtonSize::count)> icon_button_sizes{{
    {32.0, 20.0, {4.0, 6.0, 10.0}, CornerSize::medium, CornerSize::small, CornerSize::medium, CornerSize::full, 1.0},
    {40.0, 24.0, {4.0, 8.0, 14.0}, CornerSize::medium, CornerSize::small, CornerSize::medium, CornerSize::full, 1.0},
    {56.0, 24.0, {12.0, 16.0, 24.0}, CornerSize::large, CornerSize::medium, CornerSize::large, CornerSize::full, 1.0},
    {96.0, 32.0, {16.0, 32.0, 48.0}, CornerSize::extra_large, CornerSize::large, CornerSize::extra_large, CornerSize::full, 2.0},
    {136.0, 40.0, {32.0, 48.0, 72.0}, CornerSize::extra_large, CornerSize::large, CornerSize::extra_large, CornerSize::full, 3.0},
}};

struct IconButtonColors {
    std::optional<color::Role> container;
    color::Role content;
    std::optional<color::Role> outline;
};

struct IconButtonStyleTokens {
    IconButtonColors plain;
    IconButtonColors unchecked;
    IconButtonColors checked;
};

inline constexpr std::array<IconButtonStyleTokens, static_cast<std::size_t>(IconButtonStyle::count)> icon_button_styles{{
    {{color::Role::primary, color::Role::on_primary, std::nullopt},
     {color::Role::surface_container, color::Role::on_surface_variant, std::nullopt},
     {color::Role::primary, color::Role::on_primary, std::nullopt}},
    {{color::Role::secondary_container, color::Role::on_secondary_container, std::nullopt},
     {color::Role::secondary_container, color::Role::on_secondary_container, std::nullopt},
     {color::Role::secondary, color::Role::on_secondary, std::nullopt}},
    {{std::nullopt, color::Role::on_surface_variant, color::Role::outline_variant},
     {std::nullopt, color::Role::on_surface_variant, color::Role::outline_variant},
     {color::Role::inverse_surface, color::Role::inverse_on_surface, std::nullopt}},
    {{std::nullopt, color::Role::on_surface_variant, std::nullopt},
     {std::nullopt, color::Role::on_surface_variant, std::nullopt},
     {std::nullopt, color::Role::primary, std::nullopt}},
}};

constexpr const IconButtonSizeTokens& icon_button_size(IconButtonSize s) {
    return icon_button_sizes[static_cast<std::size_t>(s)];
}

constexpr const IconButtonStyleTokens& icon_button_style(IconButtonStyle s) {
    return icon_button_styles[static_cast<std::size_t>(s)];
}

constexpr double icon_button_width(IconButtonSize size, IconButtonWidth width) {
    const auto& s = icon_button_size(size);
    return s.icon_size + 2.0 * s.side_space[static_cast<std::size_t>(width)];
}

}
