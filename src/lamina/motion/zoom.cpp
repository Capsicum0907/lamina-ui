#include "lamina/motion/zoom.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::motion {

double fit_scale(const ZoomGeometry& g) {
    if (g.content_width <= 0.0 || g.content_height <= 0.0) {
        return 1.0;
    }
    return std::min(g.viewport_width / g.content_width, g.viewport_height / g.content_height);
}

double shown_width(const ZoomGeometry& g, double scale) {
    return g.content_width * fit_scale(g) * scale;
}

double shown_height(const ZoomGeometry& g, double scale) {
    return g.content_height * fit_scale(g) * scale;
}

ZoomLimits pan_limits(double viewport, double shown) {
    if (shown <= viewport) {
        const double centred = (viewport - shown) / 2.0;
        return {centred, centred};
    }
    return {viewport - shown, 0.0};
}

ZoomLimits pan_limits_x(const ZoomGeometry& g, double scale) {
    return pan_limits(g.viewport_width, shown_width(g, scale));
}

ZoomLimits pan_limits_y(const ZoomGeometry& g, double scale) {
    return pan_limits(g.viewport_height, shown_height(g, scale));
}

ZoomTransform fitted(const ZoomGeometry& g, double scale) {
    return {scale, pan_limits_x(g, scale).min, pan_limits_y(g, scale).min};
}

ZoomTransform zoom_about(const ZoomTransform& t, double fx, double fy, double scale) {
    const double k = t.scale > 0.0 ? scale / t.scale : 1.0;
    return {scale, fx - (fx - t.x) * k, fy - (fy - t.y) * k};
}

ZoomTransform pan_by(const ZoomTransform& t, double dx, double dy) {
    return {t.scale, t.x + dx, t.y + dy};
}

ZoomTransform clamp_pan(const ZoomGeometry& g, const ZoomTransform& t) {
    const ZoomLimits lx = pan_limits_x(g, t.scale);
    const ZoomLimits ly = pan_limits_y(g, t.scale);
    return {t.scale, std::clamp(t.x, lx.min, lx.max), std::clamp(t.y, ly.min, ly.max)};
}

ZoomTransform settle_target(const ZoomGeometry& g, const ZoomTransform& t, double fx, double fy, ZoomLimits scale) {
    const double s = std::clamp(t.scale, scale.min, scale.max);
    return clamp_pan(g, zoom_about(t, fx, fy, s));
}

double rubber_band(double overshoot, ZoomBand band) {
    if (overshoot <= 0.0 || band.extent <= 0.0) {
        return 0.0;
    }
    return band.extent * (1.0 - 1.0 / (overshoot * band.coefficient / band.extent + 1.0));
}

double band_into(double value, ZoomLimits limits, ZoomBand band) {
    if (value < limits.min) {
        return limits.min - rubber_band(limits.min - value, band);
    }
    if (value > limits.max) {
        return limits.max + rubber_band(value - limits.max, band);
    }
    return value;
}

double unband_from(double shown, ZoomLimits limits, ZoomBand band) {
    const auto inverse = [&](double banded) {
        if (banded <= 0.0 || band.coefficient <= 0.0) {
            return 0.0;
        }
        const double f = std::min(banded / band.extent, 1.0 - 1e-9);
        return band.extent / band.coefficient * (1.0 / (1.0 - f) - 1.0);
    };
    if (shown < limits.min) {
        return limits.min - inverse(limits.min - shown);
    }
    if (shown > limits.max) {
        return limits.max + inverse(shown - limits.max);
    }
    return shown;
}

double band_scale(double scale, ZoomLimits limits, double coefficient, double ratio) {
    if (scale <= 0.0 || limits.min <= 0.0) {
        return limits.min;
    }
    const ZoomBand band{coefficient, std::log(ratio)};
    const double banded = band_into(std::log(scale), {std::log(limits.min), std::log(limits.max)}, band);
    return std::exp(banded);
}

double unband_scale(double shown, ZoomLimits limits, double coefficient, double ratio) {
    if (shown <= 0.0 || limits.min <= 0.0) {
        return limits.min;
    }
    const ZoomBand band{coefficient, std::log(ratio)};
    return std::exp(unband_from(std::log(shown), {std::log(limits.min), std::log(limits.max)}, band));
}

bool within(double value, ZoomLimits limits, double tolerance) {
    return value >= limits.min - tolerance && value <= limits.max + tolerance;
}

bool in_bounds(const ZoomGeometry& g, const ZoomTransform& t, ZoomLimits scale, double tolerance) {
    return within(t.scale, scale, 0.0) && within(t.x, pan_limits_x(g, t.scale), tolerance) && within(t.y, pan_limits_y(g, t.scale), tolerance);
}

PanEdges pan_edges(const ZoomGeometry& g, const ZoomTransform& t, double tolerance) {
    const ZoomLimits lx = pan_limits_x(g, t.scale);
    const ZoomLimits ly = pan_limits_y(g, t.scale);
    return {t.x >= lx.max - tolerance, t.x <= lx.min + tolerance, t.y >= ly.max - tolerance, t.y <= ly.min + tolerance};
}

double content_x(const ZoomGeometry& g, const ZoomTransform& t, double viewport_x) {
    const double k = fit_scale(g) * t.scale;
    return k > 0.0 ? (viewport_x - t.x) / k : 0.0;
}

double content_y(const ZoomGeometry& g, const ZoomTransform& t, double viewport_y) {
    const double k = fit_scale(g) * t.scale;
    return k > 0.0 ? (viewport_y - t.y) / k : 0.0;
}

}
