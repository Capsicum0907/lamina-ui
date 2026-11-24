#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class ChipKind : std::size_t { assist, filter, input, suggestion, count };

struct ChipTokens {
    double height;
    CornerSize corner;
    TypeRole label;
    double icon_size;
    double avatar_size;
    double element_spacing;
    double padding;
    double input_edge_padding;
    double input_icon_padding;
    double outline_width;
    double disabled_avatar_opacity;
    double group_spacing;
    double group_line_spacing;
    double group_clip_margin;
};

inline constexpr ChipTokens chip_tokens{
    .height = 32.0,
    .corner = CornerSize::small,
    .label = TypeRole::label_large,
    .icon_size = 18.0,
    .avatar_size = 24.0,
    .element_spacing = 8.0,
    .padding = 8.0,
    .input_edge_padding = 4.0,
    .input_icon_padding = 8.0,
    .outline_width = 1.0,
    .disabled_avatar_opacity = 0.38,
    .group_spacing = 8.0,
    .group_line_spacing = 8.0,
    .group_clip_margin = 8.0,
};


struct ChipStyleTokens {
    std::optional<color::Role> container;
    std::optional<color::Role> selected_container;
    std::optional<StateColor> disabled_container;
    std::optional<StateColor> disabled_selected_container;
    color::Role label;
    color::Role selected_label;
    color::Role leading_icon;
    color::Role selected_leading_icon;
    color::Role trailing_icon;
    color::Role selected_trailing_icon;
    StateColor disabled_content;
    std::optional<color::Role> outline;
    std::optional<StateColor> disabled_outline;
    StateElevations elevation;
};

inline constexpr StateColor chip_disabled_content{color::Role::on_surface, disabled_alpha};
inline constexpr StateColor chip_disabled_container{color::Role::on_surface, 0.12};
inline constexpr StateElevations flat_chip_elevation{0.0, 0.0, 0.0, 0.0, 0.0};
inline constexpr StateElevations elevated_chip_elevation{1.0, 3.0, 1.0, 1.0, 0.0};

inline constexpr std::array<ChipStyleTokens, static_cast<std::size_t>(ChipKind::count)> flat_chip_styles{{
    {std::nullopt, std::nullopt, std::nullopt, std::nullopt, color::Role::on_surface, color::Role::on_surface, color::Role::primary,
     color::Role::primary, color::Role::primary, color::Role::primary, chip_disabled_content, color::Role::outline_variant,
     chip_disabled_container, flat_chip_elevation},
    {std::nullopt, color::Role::secondary_container, std::nullopt, chip_disabled_container, color::Role::on_surface_variant,
     color::Role::on_secondary_container, color::Role::primary, color::Role::on_secondary_container, color::Role::on_surface_variant,
     color::Role::on_secondary_container, chip_disabled_content, color::Role::outline_variant, chip_disabled_container,
     {0.0, 1.0, 0.0, 0.0, 0.0}},
    {std::nullopt, color::Role::secondary_container, std::nullopt, chip_disabled_container, color::Role::on_surface_variant,
     color::Role::on_secondary_container, color::Role::on_surface_variant, color::Role::primary, color::Role::on_surface_variant,
     color::Role::on_secondary_container, chip_disabled_content, color::Role::outline_variant, chip_disabled_container, flat_chip_elevation},
    {std::nullopt, std::nullopt, std::nullopt, std::nullopt, color::Role::on_surface_variant, color::Role::on_surface_variant,
     color::Role::primary, color::Role::primary, color::Role::primary, color::Role::primary, chip_disabled_content,
     color::Role::outline_variant, chip_disabled_container, flat_chip_elevation},
}};

inline constexpr std::array<ChipStyleTokens, static_cast<std::size_t>(ChipKind::count)> elevated_chip_styles{{
    {color::Role::surface_container_low, color::Role::surface_container_low, chip_disabled_container, chip_disabled_container,
     color::Role::on_surface, color::Role::on_surface, color::Role::primary, color::Role::primary, color::Role::primary, color::Role::primary,
     chip_disabled_content, std::nullopt, std::nullopt, elevated_chip_elevation},
    {color::Role::surface_container_low, color::Role::secondary_container, chip_disabled_container, chip_disabled_container,
     color::Role::on_surface_variant, color::Role::on_secondary_container, color::Role::primary, color::Role::on_secondary_container,
     color::Role::on_surface_variant, color::Role::on_secondary_container, chip_disabled_content, std::nullopt, std::nullopt,
     elevated_chip_elevation},
    {std::nullopt, color::Role::secondary_container, std::nullopt, chip_disabled_container, color::Role::on_surface_variant,
     color::Role::on_secondary_container, color::Role::on_surface_variant, color::Role::primary, color::Role::on_surface_variant,
     color::Role::on_secondary_container, chip_disabled_content, color::Role::outline_variant, chip_disabled_container, flat_chip_elevation},
    {color::Role::surface_container_low, color::Role::surface_container_low, chip_disabled_container, chip_disabled_container,
     color::Role::on_surface_variant, color::Role::on_surface_variant, color::Role::primary, color::Role::primary, color::Role::primary,
     color::Role::primary, chip_disabled_content, std::nullopt, std::nullopt, elevated_chip_elevation},
}};

constexpr bool chip_selectable(ChipKind kind) {
    return kind == ChipKind::filter || kind == ChipKind::input;
}

constexpr bool chip_has_elevated_variant(ChipKind kind) {
    return kind != ChipKind::input;
}

constexpr const ChipStyleTokens& chip_style(ChipKind kind, bool elevated) {
    const std::size_t i = static_cast<std::size_t>(kind);
    return elevated && chip_has_elevated_variant(kind) ? elevated_chip_styles[i] : flat_chip_styles[i];
}

struct ChipMotion {
    motion::SpringSpec slot_expand;
    motion::SpringSpec slot_shrink;
    motion::SpringSpec slot_fade_in;
    motion::SpringSpec slot_fade_out;
    motion::SpringSpec colors;
    motion::SpringSpec state_layer;
    motion::SpringSpec group_move;
    motion::SpringSpec group_presence;
};

inline constexpr ChipMotion chip_motion{
    .slot_expand = motion_scheme.fast_spatial(),
    .slot_shrink = motion_scheme.default_effects(),
    .slot_fade_in = motion_scheme.slow_effects(),
    .slot_fade_out = motion_scheme.fast_effects(),
    .colors = motion_scheme.default_effects(),
    .state_layer = motion_scheme.fast_effects(),
    .group_move = motion_scheme.default_spatial(),
    .group_presence = motion_scheme.default_effects(),
};

struct ChipSet {
    ChipTokens common = chip_tokens;
    std::array<ChipStyleTokens, static_cast<std::size_t>(ChipKind::count)> flat_styles = flat_chip_styles;
    std::array<ChipStyleTokens, static_cast<std::size_t>(ChipKind::count)> elevated_styles = elevated_chip_styles;
    ChipMotion motion = chip_motion;

    const ChipStyleTokens& style(ChipKind kind, bool elevated) const {
        const std::size_t i = static_cast<std::size_t>(kind);
        return elevated && chip_has_elevated_variant(kind) ? elevated_styles[i] : flat_styles[i];
    }
};

}
