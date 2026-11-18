#pragma once

#include <optional>
#include <span>
#include <vector>

#include "lamina/shapes/corner_rounding.hpp"
#include "lamina/shapes/cubic.hpp"
#include "lamina/shapes/feature.hpp"

namespace lamina::shapes {

class RoundedPolygon {
public:
    RoundedPolygon(std::vector<Feature> features, Point center);

    static RoundedPolygon regular(int num_vertices, float radius = 1.0f, Point center = {},
                                  CornerRounding rounding = unrounded,
                                  std::span<const CornerRounding> per_vertex_rounding = {});

    static RoundedPolygon from_vertices(std::span<const Point> vertices, CornerRounding rounding = unrounded,
                                        std::span<const CornerRounding> per_vertex_rounding = {},
                                        std::optional<Point> center = std::nullopt);

    static RoundedPolygon from_features(std::vector<Feature> features, std::optional<Point> center = std::nullopt);

    const std::vector<Feature>& features() const { return features_; }
    const std::vector<Cubic>& cubics() const { return cubics_; }
    Point center() const { return center_; }
    float center_x() const { return center_.x; }
    float center_y() const { return center_.y; }

    RoundedPolygon transformed(const PointTransformer& f) const;
    RoundedPolygon normalized() const;

    Bounds calculate_max_bounds() const;
    Bounds calculate_bounds(bool approximate = true) const;

private:
    std::vector<Feature> features_;
    Point center_;
    std::vector<Cubic> cubics_;
};

Point calculate_center(std::span<const Point> vertices);

}
