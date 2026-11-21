#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "lamina/tokens/button.hpp"
#include "lamina/tokens/chip.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/list.hpp"
#include "lamina/tokens/menu.hpp"
#include "lamina/tokens/navigation_bar.hpp"
#include "lamina/tokens/navigation_drawer.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/tokens/segmented_button.hpp"
#include "lamina/tokens/tabs.hpp"
#include "lamina/tokens/text_field.hpp"
#include "lamina/tokens/top_app_bar.hpp"

namespace lamina::tokens {

struct Density {
    int scale = 0;

    constexpr bool dense() const { return scale < 0; }
    constexpr bool operator==(const Density&) const = default;
};

inline constexpr double density_interval = 4.0;

struct DenseLength {
    double base;
    double minimum;

    constexpr double at(Density d) const { return std::clamp(base + d.scale * density_interval, minimum, base); }
    constexpr int minimum_scale() const { return static_cast<int>((minimum - base) / density_interval); }
};

constexpr DenseLength fixed_length(double v) {
    return {v, v};
}

struct DensityTokens {
    std::array<DenseLength, static_cast<std::size_t>(ButtonSize::count)> button;
    std::array<DenseLength, static_cast<std::size_t>(IconButtonSize::count)> icon_button;
    DenseLength text_field;
    DenseLength list_one_line;
    DenseLength list_two_line;
    DenseLength list_three_line;
    DenseLength chip;
    DenseLength tab;
    DenseLength tab_with_icon;
    DenseLength segmented_button;
    DenseLength top_app_bar;
    DenseLength navigation_bar;
    DenseLength rail_collapsed_item;
    DenseLength rail_expanded_item;
    DenseLength drawer_item;
    DenseLength menu_item;
    double dense_minimum_target;
};

inline constexpr DensityTokens density_tokens{
    .button = {{
        {button_sizes[0].height, 24.0},
        {button_sizes[1].height, 28.0},
        {button_sizes[2].height, 44.0},
        fixed_length(button_sizes[3].height),
        fixed_length(button_sizes[4].height),
    }},
    .icon_button = {{
        {icon_button_sizes[0].height, 24.0},
        {icon_button_sizes[1].height, 28.0},
        {icon_button_sizes[2].height, 44.0},
        fixed_length(icon_button_sizes[3].height),
        fixed_length(icon_button_sizes[4].height),
    }},
    .text_field = {outlined_text_field.height, 36.0},
    .list_one_line = {list_tokens.one_line_height, 40.0},
    .list_two_line = {list_tokens.two_line_height, 56.0},
    .list_three_line = {list_tokens.three_line_height, 56.0},
    .chip = {chip_tokens.height, 24.0},
    .tab = {tab_tokens.height, 30.0},
    .tab_with_icon = {tab_tokens.icon_and_label_height, 56.0},
    .segmented_button = {segmented_button_tokens.height, 28.0},
    .top_app_bar = {top_app_bar_tokens.collapsed_height, 48.0},
    .navigation_bar = {navigation_bar_tokens.height, 52.0},
    .rail_collapsed_item = {navigation_rail_tokens.collapsed_item_min_height, 52.0},
    .rail_expanded_item = {navigation_rail_tokens.start_indicator_height, 40.0},
    .drawer_item = {navigation_drawer_tokens.item_height, 40.0},
    .menu_item = {menu_tokens.item_height, 32.0},
    .dense_minimum_target = 0.0,
};

constexpr double minimum_target(Density d) {
    return d.dense() ? density_tokens.dense_minimum_target : minimum_interactive_size;
}

constexpr const DenseLength& button_density(ButtonSize s) {
    return density_tokens.button[static_cast<std::size_t>(s)];
}

constexpr const DenseLength& icon_button_density(IconButtonSize s) {
    return density_tokens.icon_button[static_cast<std::size_t>(s)];
}

}
