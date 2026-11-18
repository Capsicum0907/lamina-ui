#pragma once

#include <vector>

#include "lamina/shapes/cubic.hpp"

namespace lamina::shapes {

enum class FeatureKind { edge, corner };

class Feature {
public:
    static Feature edge(std::vector<Cubic> cubics);
    static Feature corner(std::vector<Cubic> cubics, bool convex = true);

    static Feature build_ignorable_feature(std::vector<Cubic> cubics);
    static Feature build_edge(const Cubic& cubic);
    static Feature build_convex_corner(std::vector<Cubic> cubics);
    static Feature build_concave_corner(std::vector<Cubic> cubics);

    const std::vector<Cubic>& cubics() const { return cubics_; }
    FeatureKind kind() const { return kind_; }
    bool convex() const { return convex_; }

    bool is_ignorable_feature() const { return kind_ == FeatureKind::edge; }
    bool is_edge() const { return kind_ == FeatureKind::edge; }
    bool is_convex_corner() const { return kind_ == FeatureKind::corner && convex_; }
    bool is_concave_corner() const { return kind_ == FeatureKind::corner && !convex_; }

    Feature transformed(const PointTransformer& f) const;
    Feature reversed() const;

private:
    Feature(std::vector<Cubic> cubics, FeatureKind kind, bool convex);

    std::vector<Cubic> cubics_;
    FeatureKind kind_;
    bool convex_;
};

}
