#include "lamina/widgets/surface.hpp"

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"

namespace lamina::widgets {

namespace {

void paint_layer(render::Canvas& canvas, const render::Rect& r, double radius, const tokens::ShadowLayer& layer,
                 color::Argb shadow_color) {
    if (layer.opacity <= 0.0 || (layer.blur <= 0.0 && layer.offset_y <= 0.0 && layer.spread <= 0.0)) {
        return;
    }
    const render::Rect grown{r.x - layer.spread, r.y - layer.spread, r.width + 2 * layer.spread, r.height + 2 * layer.spread};
    const render::Path path = render::rounded_rect_path(grown, render::uniform_radii(radius + layer.spread));
    canvas.fill_shadow(path, {0.0, layer.offset_y, layer.blur / tokens::css_blur_per_sigma,
                              color::with_alpha(shadow_color, layer.opacity * ((shadow_color >> 24) / 255.0))});
}

}

void paint_surface(render::Canvas& canvas, const render::Rect& rect, double radius, color::Argb fill, double elevation_dp,
                   color::Argb shadow_color) {
    const double level = tokens::level_for_elevation(elevation_dp);
    if (level > 0.0) {
        paint_layer(canvas, rect, radius, tokens::ambient_shadow(level), shadow_color);
        paint_layer(canvas, rect, radius, tokens::key_shadow(level), shadow_color);
    }
    canvas.fill_path(render::rounded_rect_path(rect, render::uniform_radii(radius)), fill);
}

}
