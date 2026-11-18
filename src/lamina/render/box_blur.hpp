#pragma once

#include <cstdint>

namespace lamina::render {

void gaussian_blur_prgb32(std::uint8_t* pixels, int width, int height, std::intptr_t stride, double sigma);

}
