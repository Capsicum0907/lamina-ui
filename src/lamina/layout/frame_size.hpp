#pragma once

#include <algorithm>

namespace lamina::layout {

struct PixelSize {
    int width = 0;
    int height = 0;

    constexpr bool operator==(const PixelSize&) const = default;
};

constexpr PixelSize frame_pixel_size(PixelSize reported, PixelSize last, bool minimized) {
    if (minimized && last.width > 0 && last.height > 0) {
        return last;
    }
    return {std::max(reported.width, 1), std::max(reported.height, 1)};
}

}
