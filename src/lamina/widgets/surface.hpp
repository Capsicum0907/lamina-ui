#pragma once

#include <cstddef>

#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

void paint_surface(render::Canvas& canvas, const render::Rect& rect, double radius, color::Argb fill, double elevation_dp,
                   color::Argb shadow_color);
void paint_surface(render::Canvas& canvas, const render::Rect& rect, const render::CornerRadii& radii, color::Argb fill,
                   double elevation_dp, color::Argb shadow_color);

enum class SurfaceLayer { base, overlay };

color::Role container_role(color::Role role, SurfaceLayer layer);
void paint_container(render::Canvas& canvas, const render::Rect& rect, const render::CornerRadii& radii, const color::ColorScheme& scheme,
                     color::Role fill, double elevation_dp, SurfaceLayer layer);

template <typename Style>
double state_elevation(const Style& style, const PressState& press) {
    const std::size_t level = press.pressed ? style.pressed_elevation : press.hovered ? style.hovered_elevation : style.elevation;
    return current_style().elevation.levels[level];
}

}
