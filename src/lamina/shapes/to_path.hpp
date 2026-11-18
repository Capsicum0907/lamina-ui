#pragma once

#include <span>

#include "lamina/render/path.hpp"
#include "lamina/shapes/cubic.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/rounded_polygon.hpp"

namespace lamina::shapes {

render::Path to_path(std::span<const Cubic> cubics);
render::Path to_path(const RoundedPolygon& polygon);
render::Path to_path(const Morph& morph, float progress);

}
