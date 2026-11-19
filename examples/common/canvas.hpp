#pragma once

#include <memory>

#include "lamina/render/canvas.hpp"

namespace lamina::examples {

std::unique_ptr<render::Canvas> window_canvas();

}
