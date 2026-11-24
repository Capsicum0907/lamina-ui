#pragma once

#include <array>
#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/easing.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class TopAppBarKind : std::size_t { small, center_aligned, medium, large, medium_flexible, large_flexible, count };

struct TopAppBarKindTokens {
    double expanded_height;
    double expanded_height_with_subtitle;
    bool two_rows;
    TypeRole title;
    TypeRole subtitle;
    double title_bottom_padding;
};

inline constexpr std::array<TopAppBarKindTokens, static_cast<std::size_t>(TopAppBarKind::count)> top_app_bar_kinds{{
    {64.0, 64.0, false, TypeRole::title_large, TypeRole::label_medium, 0.0},
    {64.0, 64.0, false, TypeRole::title_large, TypeRole::label_medium, 0.0},
    {112.0, 112.0, true, TypeRole::headline_small, TypeRole::label_large, 24.0},
    {152.0, 152.0, true, TypeRole::headline_medium, TypeRole::label_large, 28.0},
    {112.0, 136.0, true, TypeRole::headline_medium, TypeRole::label_large, 24.0},
    {120.0, 152.0, true, TypeRole::display_small, TypeRole::title_medium, 28.0},
}};

struct TopAppBarTokens {
    double collapsed_height;
    double horizontal_padding;
    double title_inset;
    double icon_slot;
    TypeRole collapsed_title;
    TypeRole collapsed_subtitle;
    color::Role container;
    color::Role scrolled_container;
    color::Role navigation_icon;
    color::Role title;
    color::Role subtitle;
    color::Role action_icon;
    double overlap_threshold;
    double settle_threshold;
    double settle_midpoint;
    motion::CubicBezier top_title_alpha;
    motion::CubicBezier container_easing;
    motion::SpringSpec container_spring;
    motion::SpringSpec snap_spring;
};

inline constexpr TopAppBarTokens top_app_bar_tokens{
    .collapsed_height = 64.0,
    .horizontal_padding = 4.0,
    .title_inset = 16.0 - 4.0,
    .icon_slot = 48.0,
    .collapsed_title = TypeRole::title_large,
    .collapsed_subtitle = TypeRole::label_medium,
    .container = color::Role::surface,
    .scrolled_container = color::Role::surface_container,
    .navigation_icon = color::Role::on_surface,
    .title = color::Role::on_surface,
    .subtitle = color::Role::on_surface_variant,
    .action_icon = color::Role::on_surface_variant,
    .overlap_threshold = 0.01,
    .settle_threshold = 0.01,
    .settle_midpoint = 0.5,
    .top_title_alpha = {0.8, 0.0, 0.8, 0.15},
    .container_easing = easing::legacy_accelerate,
    .container_spring = motion_scheme.default_effects(),
    .snap_spring = motion_scheme.default_effects(),
};

constexpr const TopAppBarKindTokens& top_app_bar_kind(TopAppBarKind kind) {
    return top_app_bar_kinds[static_cast<std::size_t>(kind)];
}

struct TopAppBarSet {
    std::array<TopAppBarKindTokens, static_cast<std::size_t>(TopAppBarKind::count)> kinds = top_app_bar_kinds;
    TopAppBarTokens common = top_app_bar_tokens;

    const TopAppBarKindTokens& kind(TopAppBarKind k) const { return kinds[static_cast<std::size_t>(k)]; }
};

}
