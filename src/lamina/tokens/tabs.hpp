#pragma once

#include <array>
#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class TabStyle : std::size_t { primary, secondary, count };

struct TabStyleTokens {
    color::Role container;
    color::Role active_content;
    color::Role inactive_content;
    color::Role indicator;
    double indicator_height;
    double indicator_corner;
    bool indicator_matches_content;
};

inline constexpr std::array<TabStyleTokens, static_cast<std::size_t>(TabStyle::count)> tab_styles{{
    {color::Role::surface, color::Role::primary, color::Role::on_surface_variant, color::Role::primary, 3.0, 3.0, true},
    {color::Role::surface, color::Role::on_surface, color::Role::on_surface_variant, color::Role::primary, 3.0, 0.0, false},
}};

constexpr const TabStyleTokens& tab_style(TabStyle s) {
    return tab_styles[static_cast<std::size_t>(s)];
}

struct TabTokens {
    double height;
    double icon_and_label_height;
    double icon_size;
    double horizontal_text_padding;
    double indicator_min_width;
    double text_baseline_with_icon;
    double icon_distance_from_baseline;
    double leading_icon_text_space;
    double divider_thickness;
    color::Role divider;
    TypeRole label;
    double focus_ring_corner;
};

inline constexpr TabTokens tab_tokens{
    .height = 48.0,
    .icon_and_label_height = 72.0,
    .icon_size = 24.0,
    .horizontal_text_padding = 16.0,
    .indicator_min_width = 24.0,
    .text_baseline_with_icon = 14.0,
    .icon_distance_from_baseline = 20.0,
    .leading_icon_text_space = 8.0,
    .divider_thickness = 1.0,
    .divider = color::Role::outline_variant,
    .label = TypeRole::title_small,
    .focus_ring_corner = 0.0,
};

}
