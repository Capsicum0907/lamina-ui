#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::tokens {

struct ToolbarColorTokens {
    color::Role container;
    color::Role content;
    color::Role fab_container;
    color::Role fab_content;
};

struct FloatingToolbarTokens {
    double container_size;
    double content_padding;
    double screen_offset;
    double item_slot;
    CornerSize shape;
    std::size_t expanded_elevation;
    std::size_t collapsed_elevation;
    std::size_t expanded_elevation_with_fab;
    std::size_t collapsed_elevation_with_fab;
    double fab_min_size;
    double fab_max_size;
    double toolbar_to_fab_gap;
    CornerSize fab_shape;
    std::size_t fab_elevation;
    std::size_t fab_hovered_elevation;
    double scroll_threshold;
    ToolbarColorTokens standard;
    ToolbarColorTokens vibrant;
    motion::SpringSpec expand;
};

inline constexpr FloatingToolbarTokens floating_toolbar_tokens{
    .container_size = 64.0,
    .content_padding = 8.0,
    .screen_offset = 16.0,
    .item_slot = minimum_interactive_size,
    .shape = CornerSize::full,
    .expanded_elevation = 0,
    .collapsed_elevation = 0,
    .expanded_elevation_with_fab = 1,
    .collapsed_elevation_with_fab = 0,
    .fab_min_size = 56.0,
    .fab_max_size = 80.0,
    .toolbar_to_fab_gap = 8.0,
    .fab_shape = CornerSize::large,
    .fab_elevation = 2,
    .fab_hovered_elevation = 3,
    .scroll_threshold = 40.0,
    .standard = {color::Role::surface_container, color::Role::on_surface, color::Role::primary_container, color::Role::on_primary_container},
    .vibrant = {color::Role::primary_container, color::Role::on_primary_container, color::Role::tertiary_container,
                color::Role::on_tertiary_container},
    .expand = motion_scheme.fast_spatial(),
};

struct FlexibleBottomAppBarTokens {
    double height;
    double leading_space;
    double trailing_space;
    double fixed_spacing;
    double item_slot;
    color::Role container;
    color::Role content;
    double settle_threshold;
    double settle_midpoint;
    motion::SpringSpec snap;
};

inline constexpr FlexibleBottomAppBarTokens flexible_bottom_app_bar_tokens{
    .height = 64.0,
    .leading_space = 16.0,
    .trailing_space = 16.0,
    .fixed_spacing = 32.0,
    .item_slot = minimum_interactive_size,
    .container = color::Role::surface_container,
    .content = color::Role::on_surface,
    .settle_threshold = 0.01,
    .settle_midpoint = 0.5,
    .snap = motion_scheme.fast_spatial(),
};

}
