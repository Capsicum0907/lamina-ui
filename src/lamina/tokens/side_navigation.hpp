#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/navigation_drawer.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

struct SideNavigationTokens {
    double collapsed_width;
    double expanded_min_width;
    double expanded_max_width;
    double drawer_width;
    double top_space;
    double item_inset;
    double indicator_padding;
    double icon_size;
    double collapsed_indicator_width;
    double collapsed_indicator_height;
    double icon_label_space;
    double label_fade_start;
    TypeRole label;
    color::Role container;
    color::Role indicator;
    color::Role active_icon;
    color::Role active_label;
    color::Role inactive_icon;
    color::Role inactive_label;
    color::Role state_layer;
    StateColor disabled_icon;
    StateColor disabled_label;
    motion::SpringSpec expansion;
    motion::SpringSpec selection;
    motion::SpringSpec colors;
    motion::SpringSpec state_layer_spring;
};

inline constexpr SideNavigationTokens side_navigation_tokens{
    .collapsed_width = navigation_rail_tokens.icon_only_width,
    .expanded_min_width = navigation_rail_tokens.expanded_min_width,
    .expanded_max_width = navigation_rail_tokens.expanded_max_width,
    .drawer_width = navigation_drawer_tokens.max_width,
    .top_space = navigation_drawer_tokens.top_space,
    .item_inset = navigation_drawer_tokens.item_inset,
    .indicator_padding = navigation_drawer_tokens.item_padding_start,
    .icon_size = navigation_rail_tokens.icon_size,
    .collapsed_indicator_width = navigation_rail_tokens.top_indicator_width,
    .collapsed_indicator_height = navigation_rail_tokens.top_indicator_height,
    .icon_label_space = navigation_drawer_tokens.icon_label_space,
    .label_fade_start = navigation_rail_tokens.label_hide_progress,
    .label = navigation_drawer_tokens.label_type,
    .container = navigation_rail_tokens.container,
    .indicator = navigation_rail_tokens.indicator,
    .active_icon = navigation_rail_tokens.active_icon,
    .active_label = navigation_rail_tokens.start_active_label,
    .inactive_icon = navigation_rail_tokens.inactive_icon,
    .inactive_label = navigation_rail_tokens.inactive_label,
    .state_layer = navigation_rail_tokens.state_layer,
    .disabled_icon = navigation_rail_tokens.disabled_icon,
    .disabled_label = navigation_rail_tokens.disabled_label,
    .expansion = {1.0, motion_scheme.default_spatial().stiffness},
    .selection = motion_scheme.default_spatial(),
    .colors = motion_scheme.default_effects(),
    .state_layer_spring = motion_scheme.fast_effects(),
};

}
