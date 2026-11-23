#pragma once

namespace lamina::motion {

struct ZoomTransform {
    double scale = 1.0;
    double x = 0.0;
    double y = 0.0;
};

struct ZoomGeometry {
    double viewport_width = 0.0;
    double viewport_height = 0.0;
    double content_width = 0.0;
    double content_height = 0.0;
};

struct ZoomLimits {
    double min;
    double max;
};

struct PanEdges {
    bool left = true;
    bool right = true;
    bool top = true;
    bool bottom = true;
};

struct ZoomBand {
    double coefficient;
    double extent;
};

double fit_scale(const ZoomGeometry& g);
double shown_width(const ZoomGeometry& g, double scale);
double shown_height(const ZoomGeometry& g, double scale);
ZoomLimits pan_limits(double viewport, double shown);
ZoomLimits pan_limits_x(const ZoomGeometry& g, double scale);
ZoomLimits pan_limits_y(const ZoomGeometry& g, double scale);

ZoomTransform fitted(const ZoomGeometry& g, double scale);
ZoomTransform zoom_about(const ZoomTransform& t, double focus_x, double focus_y, double scale);
ZoomTransform pan_by(const ZoomTransform& t, double dx, double dy);
ZoomTransform clamp_pan(const ZoomGeometry& g, const ZoomTransform& t);
ZoomTransform settle_target(const ZoomGeometry& g, const ZoomTransform& t, double focus_x, double focus_y, ZoomLimits scale);

double rubber_band(double overshoot, ZoomBand band);
double band_into(double value, ZoomLimits limits, ZoomBand band);
double unband_from(double shown, ZoomLimits limits, ZoomBand band);
double band_scale(double scale, ZoomLimits limits, double coefficient, double ratio);
double unband_scale(double shown, ZoomLimits limits, double coefficient, double ratio);

bool within(double value, ZoomLimits limits, double tolerance);
bool in_bounds(const ZoomGeometry& g, const ZoomTransform& t, ZoomLimits scale, double tolerance);
PanEdges pan_edges(const ZoomGeometry& g, const ZoomTransform& t, double tolerance);
double content_x(const ZoomGeometry& g, const ZoomTransform& t, double viewport_x);
double content_y(const ZoomGeometry& g, const ZoomTransform& t, double viewport_y);

}
