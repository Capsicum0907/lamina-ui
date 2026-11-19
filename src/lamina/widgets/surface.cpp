#include "lamina/widgets/surface.hpp"

#include "lamina/color/mix.hpp"
#include "lamina/tokens/elevation.hpp"

namespace lamina::widgets {

namespace {

void paint_layer(render::Canvas& canvas, const render::Rect& r, const render::CornerRadii& radii, const tokens::ShadowLayer& layer,
                 color::Argb shadow_color) {
    if (layer.opacity <= 0.0 || (layer.blur <= 0.0 && layer.offset_y <= 0.0 && layer.spread <= 0.0)) {
        return;
    }
    const double g = layer.spread;
    const render::Rect grown{r.x - g, r.y - g, r.width + 2 * g, r.height + 2 * g};
    const render::Path path = render::rounded_rect_path(
        grown, {radii.top_left + g, radii.top_right + g, radii.bottom_right + g, radii.bottom_left + g});
    canvas.fill_shadow(path, {0.0, layer.offset_y, layer.blur / tokens::css_blur_per_sigma,
                              color::scale_alpha(shadow_color, layer.opacity)});
}

}

void paint_surface(render::Canvas& canvas, const render::Rect& rect, double radius, color::Argb fill, double elevation_dp,
                   color::Argb shadow_color) {
    paint_surface(canvas, rect, render::uniform_radii(radius), fill, elevation_dp, shadow_color);
}

void paint_surface(render::Canvas& canvas, const render::Rect& rect, const render::CornerRadii& radii, color::Argb fill,
                   double elevation_dp, color::Argb shadow_color) {
    const double level = tokens::level_for_elevation(elevation_dp);
    if (level > 0.0) {
        paint_layer(canvas, rect, radii, tokens::ambient_shadow(level), shadow_color);
        paint_layer(canvas, rect, radii, tokens::key_shadow(level), shadow_color);
    }
    canvas.fill_path(render::rounded_rect_path(rect, radii), fill);
}

}
