#include <doctest/doctest.h>

#include <cmath>

#include "lamina/motion/zoom.hpp"
#include "lamina/tokens/zoom.hpp"

using namespace lamina;
using namespace lamina::motion;

namespace {

constexpr ZoomGeometry wide{400.0, 300.0, 800.0, 400.0};

double screen_x(const ZoomGeometry& g, const ZoomTransform& t, double cx) {
    return t.x + cx * fit_scale(g) * t.scale;
}

double screen_y(const ZoomGeometry& g, const ZoomTransform& t, double cy) {
    return t.y + cy * fit_scale(g) * t.scale;
}

}

TEST_CASE("zoom tokens") {
    const tokens::ZoomTokens& t = tokens::zoom;
    CHECK(t.span_slop == 16.0);
    CHECK(t.min_span == doctest::Approx(170.08).epsilon(0.001));
    CHECK(t.double_tap_timeout_seconds == 0.3);
    CHECK(t.double_tap_min_seconds == 0.04);
    CHECK(t.double_tap_slop == 100.0);
    CHECK(t.double_tap_touch_slop == 8.0);
    CHECK(t.min_scale < t.double_tap_scale);
    CHECK(t.double_tap_scale < t.max_scale);
    CHECK(t.settle_spring.damping_ratio == 1.0);
}

TEST_CASE("the content fits the viewport and is centred at scale 1") {
    CHECK(fit_scale(wide) == 0.5);
    const ZoomTransform t = fitted(wide, 1.0);
    CHECK(t.x == 0.0);
    CHECK(t.y == doctest::Approx(50.0));
    CHECK(shown_width(wide, 1.0) == 400.0);
    CHECK(shown_height(wide, 1.0) == 200.0);
    const PanEdges e = pan_edges(wide, t, 0.5);
    CHECK((e.left && e.right && e.top && e.bottom));
}

TEST_CASE("zooming about a point keeps the content under it in place") {
    const ZoomTransform t{1.3, -20.0, 10.0};
    const double fx = 123.0;
    const double fy = 77.0;
    const double cx = content_x(wide, t, fx);
    const double cy = content_y(wide, t, fy);
    for (double s : {0.5, 1.0, 2.0, 4.7}) {
        const ZoomTransform z = zoom_about(t, fx, fy, s);
        CHECK(z.scale == s);
        CHECK(screen_x(wide, z, cx) == doctest::Approx(fx));
        CHECK(screen_y(wide, z, cy) == doctest::Approx(fy));
    }
    const ZoomTransform twice = zoom_about(zoom_about(t, fx, fy, 3.0), fx, fy, 2.0);
    const ZoomTransform once = zoom_about(t, fx, fy, 2.0);
    CHECK(twice.x == doctest::Approx(once.x));
    CHECK(twice.y == doctest::Approx(once.y));
}

TEST_CASE("pan limits cover the viewport when the content is larger and centre it when smaller") {
    const ZoomLimits x = pan_limits_x(wide, 2.0);
    CHECK(x.min == -400.0);
    CHECK(x.max == 0.0);
    const ZoomLimits y = pan_limits_y(wide, 2.0);
    CHECK(y.min == -100.0);
    CHECK(y.max == 0.0);
    const ZoomLimits y_small = pan_limits_y(wide, 1.2);
    CHECK(y_small.min == y_small.max);
    CHECK(y_small.min == doctest::Approx(30.0));
    const ZoomTransform clamped = clamp_pan(wide, {2.0, 50.0, -500.0});
    CHECK(clamped.x == 0.0);
    CHECK(clamped.y == -100.0);
}

TEST_CASE("the rubber band resists more the further it is pulled and never passes its extent") {
    const ZoomBand band{0.55, 200.0};
    CHECK(rubber_band(0.0, band) == 0.0);
    CHECK(rubber_band(-5.0, band) == 0.0);
    double last = 0.0;
    for (double d = 10.0; d < 5000.0; d *= 2.0) {
        const double r = rubber_band(d, band);
        CHECK(r > last);
        CHECK(r < d);
        CHECK(r < band.extent);
        last = r;
    }
    CHECK(rubber_band(1e-6, band) / 1e-6 == doctest::Approx(0.55).epsilon(1e-3));
    const ZoomLimits limits{-100.0, 0.0};
    CHECK(band_into(-50.0, limits, band) == -50.0);
    CHECK(band_into(40.0, limits, band) > 0.0);
    CHECK(band_into(40.0, limits, band) < 40.0);
    CHECK(band_into(-160.0, limits, band) < -100.0);
    CHECK(band_into(-160.0, limits, band) > -160.0);
    for (double v : {-900.0, -160.0, -50.0, 3.0, 40.0, 700.0}) {
        CHECK(unband_from(band_into(v, limits, band), limits, band) == doctest::Approx(v));
    }
}

TEST_CASE("the scale band resists past the limits in proportion") {
    const ZoomLimits limits{1.0, 5.0};
    CHECK(band_scale(3.0, limits, 0.55, 1.5) == doctest::Approx(3.0));
    const double over = band_scale(10.0, limits, 0.55, 1.5);
    CHECK(over > 5.0);
    CHECK(over < 7.5);
    const double under = band_scale(0.5, limits, 0.55, 1.5);
    CHECK(under < 1.0);
    CHECK(under > 1.0 / 1.5);
    for (double s : {0.4, 0.9, 2.0, 6.0, 20.0}) {
        CHECK(unband_scale(band_scale(s, limits, 0.55, 1.5), limits, 0.55, 1.5) == doctest::Approx(s));
    }
}

TEST_CASE("settling clamps the scale about the focus and then the pan") {
    const ZoomTransform past{7.0, -1500.0, -600.0};
    const ZoomTransform t = settle_target(wide, past, 200.0, 150.0, {1.0, 5.0});
    CHECK(t.scale == 5.0);
    CHECK(within(t.x, pan_limits_x(wide, 5.0), 0.0));
    CHECK(within(t.y, pan_limits_y(wide, 5.0), 0.0));
    CHECK(in_bounds(wide, t, {1.0, 5.0}, 0.0));
    const ZoomTransform under = settle_target(wide, {0.6, 90.0, 100.0}, 200.0, 150.0, {1.0, 5.0});
    CHECK(under.scale == 1.0);
    CHECK(under.x == 0.0);
    CHECK(under.y == doctest::Approx(50.0));
}

TEST_CASE("pan edges say which sides of the content are showing") {
    const ZoomTransform left_top{2.0, 0.0, 0.0};
    PanEdges e = pan_edges(wide, left_top, 0.5);
    CHECK(e.left);
    CHECK(!e.right);
    CHECK(e.top);
    CHECK(!e.bottom);
    e = pan_edges(wide, {2.0, -400.0, -100.0}, 0.5);
    CHECK(!e.left);
    CHECK(e.right);
    CHECK(!e.top);
    CHECK(e.bottom);
    e = pan_edges(wide, {2.0, -200.0, -50.0}, 0.5);
    CHECK((!e.left && !e.right && !e.top && !e.bottom));
}
