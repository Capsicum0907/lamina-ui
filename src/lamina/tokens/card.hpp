#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

enum class CardStyle : std::size_t { elevated, filled, outlined, count };

struct CardStyleTokens {
    color::Role container;
    StateColor disabled_container;
    color::Role content;
    StateColor disabled_content;
    std::optional<color::Role> outline;
    std::optional<StateColor> disabled_outline;
    StateElevations elevation;
};

struct CardTokens {
    CornerSize corner;
    double outline_width;
    color::Role state_layer;
    motion::SpringSpec state_layer_spring;
};

inline constexpr CardTokens card_tokens{
    .corner = CornerSize::medium,
    .outline_width = 1.0,
    .state_layer = color::Role::on_surface,
    .state_layer_spring = motion_scheme.fast_effects(),
};

inline constexpr std::array<CardStyleTokens, static_cast<std::size_t>(CardStyle::count)> card_styles{{
    {color::Role::surface_container_low, {color::Role::surface, disabled_alpha, false, color::Role::surface}, color::Role::on_surface,
     {color::Role::on_surface, disabled_alpha}, std::nullopt, std::nullopt, {1.0, 3.0, 1.0, 1.0, 1.0}},
    {color::Role::surface_container_highest,
     {color::Role::surface_variant, disabled_alpha, false, color::Role::surface_container_highest},
     color::Role::on_surface,
     {color::Role::on_surface, disabled_alpha},
     std::nullopt,
     std::nullopt,
     {0.0, 1.0, 0.0, 0.0, 0.0}},
    {color::Role::surface, {color::Role::surface}, color::Role::on_surface, {color::Role::on_surface, disabled_alpha},
     color::Role::outline_variant, StateColor{color::Role::outline, 0.12, false, color::Role::surface_container_low},
     {0.0, 0.0, 0.0, 0.0, 0.0}},
}};

constexpr const CardStyleTokens& card_style(CardStyle style) {
    return card_styles[static_cast<std::size_t>(style)];
}

}
