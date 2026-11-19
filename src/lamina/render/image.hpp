#pragma once

#include <vector>

#include "lamina/color/argb.hpp"

namespace lamina::render {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<color::Argb> pixels;
};

}
