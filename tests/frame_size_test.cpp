#include <doctest/doctest.h>

#include "lamina/layout/frame_size.hpp"

using namespace lamina;

TEST_CASE("a minimized window keeps the last frame size instead of the size the system reports") {
    const layout::PixelSize last{1440, 900};
    CHECK(layout::frame_pixel_size({160, 28}, last, true) == last);
    CHECK(layout::frame_pixel_size({0, 0}, last, true) == last);
}

TEST_CASE("a window that is not minimized takes the reported size") {
    CHECK(layout::frame_pixel_size({1920, 1032}, {1440, 900}, false) == layout::PixelSize{1920, 1032});
    CHECK(layout::frame_pixel_size({160, 28}, {1440, 900}, false) == layout::PixelSize{160, 28});
}

TEST_CASE("a frame is at least one pixel, also when a window starts minimized") {
    CHECK(layout::frame_pixel_size({0, 0}, {}, false) == layout::PixelSize{1, 1});
    CHECK(layout::frame_pixel_size({160, 28}, {}, true) == layout::PixelSize{160, 28});
    CHECK(layout::frame_pixel_size({0, 0}, {}, true) == layout::PixelSize{1, 1});
}
