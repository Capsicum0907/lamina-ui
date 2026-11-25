#include "lamina/style/style.hpp"
#include "lamina/widgets/surface.hpp"

#include <algorithm>

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
    canvas.fill_shadow(path, {0.0, layer.offset_y, layer.blur / current_style().elevation.blur_per_sigma,
                              color::scale_alpha(shadow_color, layer.opacity)});
}

}

color::Role container_role(color::Role role, SurfaceLayer layer) {
    const Style& s = current_style();
    if (layer == SurfaceLayer::overlay && s.layering.mode == tokens::Layering::recede) {
        return s.layering.receded_container;
    }
    return s.containers.fill == tokens::ContainerFill::outlined ? s.containers.outlined_fill : role;
}

void paint_container(render::Canvas& canvas, const render::Rect& rect, const render::CornerRadii& radii, const color::ColorScheme& scheme,
                     color::Role fill, double elevation_dp, SurfaceLayer layer) {
    paint_surface(canvas, rect, radii, scheme[container_role(fill, layer)], elevation_dp, scheme[color::Role::shadow]);
    const tokens::ContainerTokens& c = current_style().containers;
    if (c.fill != tokens::ContainerFill::outlined || c.outline_width <= 0.0) {
        return;
    }
    const double w = c.outline_width;
    const render::Rect inner{rect.x + w, rect.y + w, rect.width - 2.0 * w, rect.height - 2.0 * w};
    auto shrink = [w](double r) { return std::max(0.0, r - w); };
    render::Path ring = render::rounded_rect_path(rect, radii);
    ring.append(render::rounded_rect_path(inner, {shrink(radii.top_left), shrink(radii.top_right), shrink(radii.bottom_right), shrink(radii.bottom_left)}));
    canvas.fill_path(ring, scheme[c.outline], render::FillRule::even_odd);
}

void paint_surface(render::Canvas& canvas, const render::Rect& rect, double radius, color::Argb fill, double elevation_dp,
                   color::Argb shadow_color) {
    paint_surface(canvas, rect, render::uniform_radii(radius), fill, elevation_dp, shadow_color);
}

void paint_surface(render::Canvas& canvas, const render::Rect& rect, const render::CornerRadii& radii, color::Argb fill,
                   double elevation_dp, color::Argb shadow_color) {
    const double level = current_style().elevation.level_for(elevation_dp);
    if (level > 0.0) {
        paint_layer(canvas, rect, radii, current_style().elevation.ambient_shadow(level), shadow_color);
        paint_layer(canvas, rect, radii, current_style().elevation.key_shadow(level), shadow_color);
    }
    canvas.fill_path(render::rounded_rect_path(rect, radii), fill);
}

}
