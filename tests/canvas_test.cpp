#include <doctest/doctest.h>

#include <cstdint>

#include "lamina_thorvg/canvas.hpp"

using namespace lamina::render;
using lamina::color::Argb;

namespace {

constexpr Argb white = 0xffffffff;
constexpr Argb red = 0xffff0000;

int channel(Argb c, int shift) {
    return static_cast<int>((c >> shift) & 0xff);
}

bool near(Argb a, Argb b, int tolerance) {
    for (int shift : {0, 8, 16, 24}) {
        const int d = channel(a, shift) - channel(b, shift);
        if (d > tolerance || d < -tolerance) {
            return false;
        }
    }
    return true;
}

}

TEST_CASE("fit_radii scales radii down to fit the rectangle") {
    const Rect r{0, 0, 100, 40};
    const auto fitted = fit_radii(r, uniform_radii(30));
    CHECK(fitted.top_left == doctest::Approx(20));
    CHECK(fitted.bottom_right == doctest::Approx(20));
    const auto untouched = fit_radii(r, uniform_radii(10));
    CHECK(untouched.top_left == doctest::Approx(10));
}

TEST_CASE("transform stack composes translate and scale") {
    lamina::thorvg::Canvas canvas(4, 4);
    canvas.save();
    canvas.scale(2.0);
    canvas.translate(3.0, 1.0);
    const Point p = canvas.transform().apply({1.0, 1.0});
    CHECK(p.x == doctest::Approx(8.0));
    CHECK(p.y == doctest::Approx(4.0));
    canvas.restore();
    CHECK(canvas.transform().apply({1.0, 1.0}).x == doctest::Approx(1.0));
}

TEST_CASE_TEMPLATE("rounded rectangles cover the inside and leave the corners", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(100, 100);
    canvas.clear(white);
    canvas.fill_rounded_rect({10, 10, 80, 80}, uniform_radii(20), red);

    CHECK(canvas.pixel(50, 50) == red);
    CHECK(canvas.pixel(50, 11) == red);
    CHECK(canvas.pixel(11, 50) == red);
    CHECK(canvas.pixel(12, 12) == white);
    CHECK(canvas.pixel(87, 87) == white);
    CHECK(canvas.pixel(5, 5) == white);

    const Argb edge = canvas.pixel(15, 15);
    CHECK(edge != red);
    CHECK(edge != white);
}

TEST_CASE_TEMPLATE("translucent fills blend over what is below", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(10, 10);
    canvas.clear(white);
    canvas.fill_rounded_rect({0, 0, 10, 10}, uniform_radii(0), 0x80000000);
    CHECK(near(canvas.pixel(5, 5), 0xff7f7f7f, 2));
}

TEST_CASE_TEMPLATE("scale maps dp to pixels", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(40, 40);
    canvas.clear(white);
    canvas.scale(2.0);
    canvas.fill_rounded_rect({5, 5, 10, 10}, uniform_radii(0), red);
    CHECK(canvas.pixel(11, 11) == red);
    CHECK(canvas.pixel(29, 29) == red);
    CHECK(canvas.pixel(31, 31) == white);
}

TEST_CASE_TEMPLATE("fill_rect paints the same pixels as a rectangle path", CanvasType, lamina::thorvg::Canvas) {
    const Rect r{3.3, 4.7, 17.45, 9.6};
    CanvasType by_rect(48, 48);
    CanvasType by_path(48, 48);
    for (CanvasType* c : {&by_rect, &by_path}) {
        c->clear(white);
        c->translate(0.4, 0.15);
        c->scale(1.75);
    }
    by_rect.fill_rect(r, 0xc0ff0000);
    by_path.fill_path(rounded_rect_path(r, uniform_radii(0.0)), 0xc0ff0000);
    int differing = 0;
    for (int y = 0; y < 48; ++y) {
        for (int x = 0; x < 48; ++x) {
            differing += by_rect.pixel(x, y) != by_path.pixel(x, y);
        }
    }
    CHECK(differing == 0);
    const Argb edge = by_rect.pixel(6, 20);
    CHECK(edge != white);
    CHECK(edge != by_rect.pixel(20, 20));
}

TEST_CASE("point operators, length and unit") {
    const Point a{1.0, 2.0};
    const Point b{4.0, 6.0};
    CHECK(length(b - a) == 5.0);
    const Point u = unit(b - a);
    CHECK(u.x == doctest::Approx(0.6));
    CHECK(u.y == doctest::Approx(0.8));
    CHECK(unit({0.0, 0.0}).x == 0.0);
    const Point m = a + b * 0.5 - Point{1.0, 1.0};
    CHECK(m.x == 2.0);
    CHECK(m.y == 4.0);
    CHECK(left_normal({1.0, 0.0}).y == 1.0);
    CHECK(cross({1.0, 0.0}, {0.0, 1.0}) == 1.0);
    CHECK((-a / 2.0).y == -1.0);
    const Rect c = circle_rect({10.0, 10.0}, 4.0);
    CHECK(c.x == 6.0);
    CHECK(c.width == 8.0);
}

TEST_CASE_TEMPLATE("shadows fall in the offset direction", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(120, 120);
    canvas.clear(white);
    const Path shape = rounded_rect_path({30, 30, 60, 60}, uniform_radii(8));
    canvas.fill_shadow(shape, {0.0, 8.0, 4.0, 0xff000000});
    canvas.fill_path(shape, red);
    CHECK(canvas.pixel(60, 60) == red);
    const Argb below = canvas.pixel(60, 93);
    const Argb above = canvas.pixel(60, 27);
    CAPTURE(below);
    CAPTURE(above);
    CHECK(channel(below, 16) < 200);
    CHECK(channel(above, 16) > channel(below, 16));
}
