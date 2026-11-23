#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina_sdl3/window.hpp"

namespace lamina::sdl3 {

std::optional<render::Rect> paint_frame_stats(Window& window, render::Canvas& canvas, const text::FontStack& fonts, const color::ColorScheme& scheme,
                                              const Frame& frame);

}
