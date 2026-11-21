#include <doctest/doctest.h>

#include <string>

#include "lamina/layout/window_size_class.hpp"

using namespace lamina;
using layout::WindowHeightClass;
using layout::WindowWidthClass;

TEST_CASE("window width classes follow window-core's breakpoints") {
    CHECK(layout::compute_window_size_class(0.0, 0.0).width_class() == WindowWidthClass::compact);
    CHECK(layout::compute_window_size_class(599.9, 800.0).width_class() == WindowWidthClass::compact);
    CHECK(layout::compute_window_size_class(600.0, 800.0).width_class() == WindowWidthClass::medium);
    CHECK(layout::compute_window_size_class(839.0, 800.0).width_class() == WindowWidthClass::medium);
    CHECK(layout::compute_window_size_class(840.0, 800.0).width_class() == WindowWidthClass::expanded);
    CHECK(layout::compute_window_size_class(1199.5, 800.0).width_class() == WindowWidthClass::expanded);
    CHECK(layout::compute_window_size_class(1200.0, 800.0).width_class() == WindowWidthClass::large);
    CHECK(layout::compute_window_size_class(1600.0, 800.0).width_class() == WindowWidthClass::extra_large);
    CHECK(layout::compute_window_size_class(4000.0, 800.0).min_width_dp == 1600);
}

TEST_CASE("window height classes follow window-core's breakpoints") {
    CHECK(layout::compute_window_size_class(800.0, 479.9).height_class() == WindowHeightClass::compact);
    CHECK(layout::compute_window_size_class(800.0, 480.0).height_class() == WindowHeightClass::medium);
    CHECK(layout::compute_window_size_class(800.0, 899.0).height_class() == WindowHeightClass::medium);
    CHECK(layout::compute_window_size_class(800.0, 900.0).height_class() == WindowHeightClass::expanded);
    CHECK(layout::compute_window_size_class(-10.0, -10.0) == layout::WindowSizeClass{0, 0});
}

TEST_CASE("a size class compares against breakpoints as window-core does") {
    const layout::WindowSizeClass c = layout::compute_window_size_class(1000.0, 700.0);
    CHECK(c.min_width_dp == 840);
    CHECK(c.min_height_dp == 480);
    CHECK(c.is_width_at_least(600));
    CHECK(c.is_width_at_least(WindowWidthClass::expanded));
    CHECK_FALSE(c.is_width_at_least(WindowWidthClass::large));
    CHECK(c.is_height_at_least(WindowHeightClass::medium));
    CHECK_FALSE(c.is_height_at_least(WindowHeightClass::expanded));
    CHECK(std::string(layout::name(c.width_class())) == "expanded");
    CHECK(std::string(layout::name(WindowWidthClass::extra_large)) == "extra-large");
    static_assert(layout::compute_window_size_class(1300.0, 1000.0).width_class() == WindowWidthClass::large);
}
