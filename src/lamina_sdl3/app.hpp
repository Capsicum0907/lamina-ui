#pragma once

#include <functional>

#include "lamina/color/argb.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina_sdl3/window.hpp"

namespace lamina::sdl3 {

using UpdateFrame = std::function<color::Argb(const Frame& frame)>;
using PaintFrame = std::function<bool(render::Canvas& canvas, const Frame& frame)>;

void run_app(Window& window, const UpdateFrame& update, const PaintFrame& paint);
void run_app(const WindowOptions& options, const UpdateFrame& update, const PaintFrame& paint);

}
