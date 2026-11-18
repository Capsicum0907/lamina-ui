#pragma once

#include <array>
#include <cstddef>

#include "lamina/shapes/rounded_polygon.hpp"

namespace lamina::shapes {

enum class MaterialShape {
    circle,
    square,
    slanted,
    arch,
    fan,
    arrow,
    semi_circle,
    oval,
    pill,
    triangle,
    diamond,
    clam_shell,
    pentagon,
    gem,
    sunny,
    very_sunny,
    cookie_4_sided,
    cookie_6_sided,
    cookie_7_sided,
    cookie_9_sided,
    cookie_12_sided,
    ghostish,
    clover_4_leaf,
    clover_8_leaf,
    burst,
    soft_burst,
    boom,
    soft_boom,
    flower,
    puffy,
    puffy_diamond,
    pixel_circle,
    pixel_triangle,
    bun,
    heart,
};

inline constexpr std::size_t material_shape_count = static_cast<std::size_t>(MaterialShape::heart) + 1;

const std::array<MaterialShape, material_shape_count>& all_material_shapes();

const RoundedPolygon& material_shape(MaterialShape shape);

RoundedPolygon material_shape_unnormalized(MaterialShape shape);

}
