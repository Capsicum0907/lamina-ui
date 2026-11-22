#include <doctest/doctest.h>

#include "lamina/layout/safe_area.hpp"

using namespace lamina;

TEST_CASE("safe area insets are the distances from the window edges to the safe rectangle, in dp") {
    const render::Rect window{0.0, 0.0, 1080.0, 2400.0};
    const render::Rect safe{0.0, 63.0, 1080.0, 2274.0};
    const layout::Insets insets = layout::safe_area_insets(window, safe, 1.0 / 2.625);
    CHECK(insets.left == doctest::Approx(0.0));
    CHECK(insets.top == doctest::Approx(24.0));
    CHECK(insets.right == doctest::Approx(0.0));
    CHECK(insets.bottom == doctest::Approx(24.0));
}

TEST_CASE("safe area insets cover a cutout on the side and never go negative") {
    const render::Rect window{0.0, 0.0, 800.0, 400.0};
    const layout::Insets side = layout::safe_area_insets(window, {48.0, 0.0, 700.0, 400.0}, 1.0);
    CHECK(side.left == doctest::Approx(48.0));
    CHECK(side.right == doctest::Approx(52.0));
    const layout::Insets outside = layout::safe_area_insets(window, {-10.0, -10.0, 820.0, 420.0}, 1.0);
    CHECK(outside.left == 0.0);
    CHECK(outside.top == 0.0);
    CHECK(outside.right == 0.0);
    CHECK(outside.bottom == 0.0);
}

TEST_CASE("a safe area equal to the window gives no insets") {
    const render::Rect window{0.0, 0.0, 1440.0, 860.0};
    const layout::Insets none = layout::safe_area_insets(window, window, 1.5);
    CHECK(none.left == 0.0);
    CHECK(none.top == 0.0);
    CHECK(none.right == 0.0);
    CHECK(none.bottom == 0.0);
}

TEST_CASE("inset_rect shrinks a rectangle by the insets and clamps at zero size") {
    const render::Rect r = layout::inset_rect({0.0, 0.0, 412.0, 915.0}, {0.0, 24.0, 0.0, 48.0});
    CHECK(r.x == 0.0);
    CHECK(r.y == 24.0);
    CHECK(r.width == 412.0);
    CHECK(r.height == 843.0);
    const render::Rect tiny = layout::inset_rect({10.0, 10.0, 20.0, 20.0}, layout::Insets::all(30.0));
    CHECK(tiny.width == 0.0);
    CHECK(tiny.height == 0.0);
}
