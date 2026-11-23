#include <doctest/doctest.h>

#include "lamina/render/layer_plan.hpp"

using lamina::render::LayerPlan;
using lamina::render::Rect;

TEST_CASE("paints stay in the bottom layer until they overlap an image") {
    LayerPlan plan(3);
    CHECK(plan.place_paint({0, 0, 100, 100}) == 0);
    CHECK(plan.place_image({10, 10, 50, 50}) == 0u);
    CHECK(plan.place_paint({70, 70, 10, 10}) == 0);
    CHECK(plan.place_paint({40, 40, 30, 30}) == 1);
    CHECK(plan.layers() == 2);
}

TEST_CASE("a paint that overlaps a raised paint is raised with it") {
    LayerPlan plan(3);
    plan.place_paint({0, 0, 100, 100});
    plan.place_image({0, 0, 50, 50});
    CHECK(plan.place_paint({40, 40, 20, 20}) == 1);
    CHECK(plan.place_paint({55, 55, 20, 20}) == 1);
    CHECK(plan.place_paint({80, 0, 10, 10}) == 0);
}

TEST_CASE("an image over a raised paint goes into a later batch") {
    LayerPlan plan(3);
    plan.place_paint({0, 0, 100, 100});
    CHECK(plan.place_image({0, 0, 50, 50}) == 0u);
    CHECK(plan.place_paint({40, 40, 20, 20}) == 1);
    CHECK(plan.place_image({50, 50, 20, 20}) == 1u);
    CHECK(plan.place_paint({60, 60, 5, 5}) == 2);
}

TEST_CASE("an image that would need more layers than allowed is refused") {
    LayerPlan plan(2);
    plan.place_paint({0, 0, 100, 100});
    CHECK(plan.place_image({0, 0, 50, 50}) == 0u);
    CHECK(plan.place_paint({40, 40, 20, 20}) == 1);
    CHECK_FALSE(plan.place_image({45, 45, 10, 10}).has_value());
    CHECK(plan.place_image({0, 70, 10, 10}) == 0u);
}

TEST_CASE("one layer allows no image") {
    LayerPlan plan(1);
    CHECK_FALSE(plan.place_image({0, 0, 10, 10}).has_value());
}

TEST_CASE("images that do not overlap share the first batch and later images above earlier ones stay in order") {
    LayerPlan plan(3);
    plan.place_paint({0, 0, 400, 400});
    CHECK(plan.place_image({0, 0, 100, 100}) == 0u);
    CHECK(plan.place_image({110, 0, 100, 100}) == 0u);
    CHECK(plan.place_image({50, 50, 100, 100}) == 0u);
    CHECK(plan.place_paint({120, 120, 5, 5}) == 1);
}

TEST_CASE("layer bounds are the union of the paints placed in them") {
    LayerPlan plan(3);
    plan.place_paint({0, 0, 100, 100});
    plan.place_image({0, 0, 100, 100});
    plan.place_paint({10, 10, 10, 10});
    plan.place_paint({50, 60, 10, 10});
    const auto b = plan.bounds(1);
    REQUIRE(b.has_value());
    CHECK(b->x == doctest::Approx(10));
    CHECK(b->y == doctest::Approx(10));
    CHECK(b->width == doctest::Approx(50));
    CHECK(b->height == doctest::Approx(60));
    plan.reset();
    CHECK_FALSE(plan.bounds(1).has_value());
    CHECK(plan.layers() == 1);
}

TEST_CASE("empty rectangles overlap nothing") {
    LayerPlan plan(3);
    plan.place_paint({0, 0, 100, 100});
    plan.place_image({0, 0, 100, 100});
    CHECK(plan.place_paint({10, 10, 0, 10}) == 0);
}
