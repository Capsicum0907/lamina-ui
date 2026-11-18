#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"

namespace lamina::widgets {

void paint_surface(render::Canvas& canvas, const render::Rect& rect, double radius, color::Argb fill, double elevation_dp,
                   color::Argb shadow_color);

}
