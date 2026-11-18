#pragma once

#include <optional>
#include <span>

#include "lamina/shapes/rounded_polygon.hpp"

namespace lamina::shapes {

inline constexpr int default_circle_vertices = 8;

RoundedPolygon circle(int num_vertices = default_circle_vertices, float radius = 1.0f, Point center = {});

RoundedPolygon rectangle(float width = 2.0f, float height = 2.0f, CornerRounding rounding = unrounded,
                         std::span<const CornerRounding> per_vertex_rounding = {}, Point center = {});

RoundedPolygon star(int num_vertices_per_radius, float radius = 1.0f, float inner_radius = 0.5f,
                    CornerRounding rounding = unrounded, std::optional<CornerRounding> inner_rounding = std::nullopt,
                    std::span<const CornerRounding> per_vertex_rounding = {}, Point center = {});

RoundedPolygon pill(float width = 2.0f, float height = 1.0f, float smoothing = 0.0f, Point center = {});

struct PillStarOptions {
    float width = 2.0f;
    float height = 1.0f;
    int num_vertices_per_radius = 8;
    float inner_radius_ratio = 0.5f;
    CornerRounding rounding = unrounded;
    std::optional<CornerRounding> inner_rounding = std::nullopt;
    std::span<const CornerRounding> per_vertex_rounding = {};
    float vertex_spacing = 0.5f;
    float start_location = 0.0f;
    Point center = {};
};

RoundedPolygon pill_star(const PillStarOptions& options = {});

}
