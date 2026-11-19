#include "common/canvas.hpp"

#include "lamina_thorvg/canvas.hpp"

namespace lamina::examples {

namespace {

constexpr int unsized_pixels = 1;

}

std::unique_ptr<render::Canvas> window_canvas() {
    return std::make_unique<thorvg::Canvas>(unsized_pixels, unsized_pixels);
}

}
