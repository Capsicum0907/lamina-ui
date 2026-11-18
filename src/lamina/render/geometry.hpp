#pragma once

namespace lamina::render {

struct Point {
    double x;
    double y;
};

struct Rect {
    double x;
    double y;
    double width;
    double height;
};

struct CornerRadii {
    double top_left;
    double top_right;
    double bottom_right;
    double bottom_left;
};

constexpr CornerRadii uniform_radii(double r) {
    return {r, r, r, r};
}

CornerRadii fit_radii(const Rect& rect, CornerRadii radii);

struct Affine {
    double a = 1.0;
    double b = 0.0;
    double c = 0.0;
    double d = 1.0;
    double tx = 0.0;
    double ty = 0.0;

    constexpr Point apply(Point p) const {
        return {a * p.x + c * p.y + tx, b * p.x + d * p.y + ty};
    }

    constexpr Affine then_local(const Affine& m) const {
        return {
            a * m.a + c * m.b,
            b * m.a + d * m.b,
            a * m.c + c * m.d,
            b * m.c + d * m.d,
            a * m.tx + c * m.ty + tx,
            b * m.tx + d * m.ty + ty,
        };
    }
};

}
