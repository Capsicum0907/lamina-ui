#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
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
    {32.0, CornerSize::medium, CornerSize::small, 20.0, 4.0, 12.0, 12.0, 1.0, TypeRole::label_large},
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
    std::optional<StateColor> disabled_container;
    StateColor disabled_label;
    std::optional<StateColor> disabled_outline;
    std::size_t disabled_elevation;
};

inline constexpr std::array<ButtonStyleTokens, static_cast<std::size_t>(ButtonStyle::count)> button_styles{{
    {color::Role::primary, color::Role::on_primary, std::nullopt, 0, 1, 0, StateColor{color::Role::on_surface, 0.1},
     StateColor{color::Role::on_surface_variant, 0.38}, std::nullopt, 0},
    {color::Role::secondary_container, color::Role::on_secondary_container, std::nullopt, 0, 1, 0,
     StateColor{color::Role::on_surface, 0.12}, StateColor{color::Role::on_surface, 0.38}, std::nullopt, 0},
    {std::nullopt, color::Role::on_surface_variant, color::Role::outline_variant, 0, 0, 0, std::nullopt,
     StateColor{color::Role::on_surface_variant, 0.38}, StateColor{color::Role::outline_variant, 0.1}, 0},
    {std::nullopt, color::Role::primary, std::nullopt, 0, 0, 0, std::nullopt, StateColor{color::Role::on_surface_variant, 0.38},
     std::nullopt, 0},
    {color::Role::surface_container_low, color::Role::primary, std::nullopt, 1, 2, 1, StateColor{color::Role::on_surface, 0.1},
     StateColor{color::Role::on_surface_variant, 0.38}, std::nullopt, 0},
}};

struct ToggleButtonColorTokens {
    std::optional<color::Role> container;
    color::Role content;
    color::Role checked_container;
    color::Role checked_content;
    StateColor disabled_container;
    StateColor disabled_content;
};

inline constexpr std::array<std::optional<ToggleButtonColorTokens>, static_cast<std::size_t>(ButtonStyle::count)> toggle_button_styles{{
    ToggleButtonColorTokens{color::Role::surface_container, color::Role::on_surface_variant, color::Role::primary, color::Role::on_primary,
                            StateColor{color::Role::on_surface, 0.1}, StateColor{color::Role::on_surface_variant, 0.38}},
    ToggleButtonColorTokens{color::Role::secondary_container, color::Role::on_secondary_container, color::Role::secondary,
                            color::Role::on_secondary, StateColor{color::Role::on_surface, 0.1}, StateColor{color::Role::on_surface_variant, 0.38}},
    ToggleButtonColorTokens{std::nullopt, color::Role::on_surface_variant, color::Role::inverse_surface, color::Role::inverse_on_surface,
                            StateColor{color::Role::outline_variant, 0.1}, StateColor{color::Role::on_surface_variant, 0.38}},
    std::nullopt,
    ToggleButtonColorTokens{color::Role::surface_container_low, color::Role::primary, color::Role::primary, color::Role::on_primary,
                            StateColor{color::Role::on_surface, 0.1}, StateColor{color::Role::on_surface_variant, 0.38}},
}};

struct ToggleButtonMotion {
    motion::SpringSpec shape;
    motion::SpringSpec border_width;
    motion::SpringSpec colors;
};

inline constexpr ToggleButtonMotion toggle_button_motion{
    .shape = motion_scheme.fast_spatial(),
    .border_width = motion_scheme.fast_spatial(),
    .colors = motion_scheme.default_effects(),
};

constexpr const ButtonSizeTokens& button_size(ButtonSize s) {
    return button_sizes[static_cast<std::size_t>(s)];
}

constexpr const ButtonStyleTokens& button_style(ButtonStyle s) {
    return button_styles[static_cast<std::size_t>(s)];
}

constexpr const std::optional<ToggleButtonColorTokens>& toggle_button_style(ButtonStyle s) {
    return toggle_button_styles[static_cast<std::size_t>(s)];
}

constexpr Corners toggle_button_rest_corners(ButtonSize) {
    return all_corners(CornerSize::full);
}

constexpr Corners toggle_button_pressed_corners(ButtonSize s) {
    return all_corners(button_size(s).pressed_corner);
}

constexpr Corners toggle_button_checked_corners(ButtonSize s) {
    return all_corners(button_size(s).square_corner);
}

struct ButtonSet {
    std::array<ButtonSizeTokens, static_cast<std::size_t>(ButtonSize::count)> sizes = button_sizes;
    double min_width = button_min_width;
    std::array<ButtonStyleTokens, static_cast<std::size_t>(ButtonStyle::count)> styles = button_styles;
    std::array<std::optional<ToggleButtonColorTokens>, static_cast<std::size_t>(ButtonStyle::count)> toggle_styles = toggle_button_styles;
    ToggleButtonMotion toggle_motion = toggle_button_motion;

    const ButtonSizeTokens& size(ButtonSize s) const { return sizes[static_cast<std::size_t>(s)]; }
    const ButtonStyleTokens& style(ButtonStyle s) const { return styles[static_cast<std::size_t>(s)]; }
    const std::optional<ToggleButtonColorTokens>& toggle_style(ButtonStyle s) const { return toggle_styles[static_cast<std::size_t>(s)]; }
    Corners toggle_rest_corners(ButtonSize) const { return all_corners(CornerSize::full); }
    Corners toggle_pressed_corners(ButtonSize s) const { return all_corners(size(s).pressed_corner); }
    Corners toggle_checked_corners(ButtonSize s) const { return all_corners(size(s).square_corner); }
};

}
