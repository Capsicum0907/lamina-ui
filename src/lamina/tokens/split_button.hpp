#pragma once

#include <array>
#include <cstddef>
#include <numbers>

#include "lamina/builtin_resources.hpp"
#include "lamina/icons/symbol.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct SplitButtonSizeTokens {
    double height;
    CornerSize inner_corner;
    CornerSize inner_pressed_corner;
    double leading_leading_space;
    double leading_trailing_space;
    double trailing_leading_space;
    double trailing_trailing_space;
    double trailing_icon_size;
};

inline constexpr std::array<SplitButtonSizeTokens, static_cast<std::size_t>(ButtonSize::count)> split_button_sizes{{
    {32.0, CornerSize::extra_small, CornerSize::small, 12.0, 10.0, 13.0, 13.0, 22.0},
    {40.0, CornerSize::extra_small, CornerSize::medium, 16.0, 12.0, 13.0, 13.0, 22.0},
    {56.0, CornerSize::extra_small, CornerSize::medium, 24.0, 24.0, 15.0, 15.0, 26.0},
    {96.0, CornerSize::small, CornerSize::large_increased, 48.0, 48.0, 29.0, 29.0, 38.0},
    {136.0, CornerSize::medium, CornerSize::large_increased, 64.0, 64.0, 43.0, 43.0, 50.0},
}};

struct SplitButtonTokens {
    double between_space;
    double min_width;
    double checked_overlay;
    double chevron_turn;
    icons::Symbol chevron;
    motion::SpringSpec shape_spring;
    motion::SpringSpec chevron_spring;
};

inline constexpr SplitButtonTokens split_button_tokens{
    .between_space = 2.0,
    .min_width = 48.0,
    .checked_overlay = state_layer_opacity::pressed,
    .chevron_turn = std::numbers::pi,
    .chevron = builtin::icons::keyboard_arrow_down,
    .shape_spring = motion_scheme.default_effects(),
    .chevron_spring = {1.0, 1500.0},
};

constexpr const SplitButtonSizeTokens& split_button_size(ButtonSize s) {
    return split_button_sizes[static_cast<std::size_t>(s)];
}

constexpr Corners split_leading_corners(CornerSize inner) {
    return {CornerSize::full, inner, inner, CornerSize::full};
}

constexpr Corners split_trailing_corners(CornerSize inner) {
    return {inner, CornerSize::full, CornerSize::full, inner};
}

struct SplitButtonSet {
    std::array<SplitButtonSizeTokens, static_cast<std::size_t>(ButtonSize::count)> sizes = split_button_sizes;
    SplitButtonTokens common = split_button_tokens;

    const SplitButtonSizeTokens& size(ButtonSize s) const { return sizes[static_cast<std::size_t>(s)]; }
};

}
