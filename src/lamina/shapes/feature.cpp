#include "lamina/shapes/feature.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace lamina::shapes {

namespace {

bool is_continuous(const std::vector<Cubic>& cubics) {
    const Cubic* prev = &cubics.front();
    for (std::size_t i = 1; i < cubics.size(); ++i) {
        const Cubic& cubic = cubics[i];
        if (std::abs(cubic.anchor0_x() - prev->anchor1_x()) > distance_epsilon ||
            std::abs(cubic.anchor0_y() - prev->anchor1_y()) > distance_epsilon) {
            return false;
        }
        prev = &cubic;
    }
    return true;
}

Feature validated(Feature feature) {
    if (feature.cubics().empty()) {
        throw std::invalid_argument("Features need at least one cubic.");
    }
    if (!is_continuous(feature.cubics())) {
        throw std::invalid_argument(
            "Feature must be continuous, with the anchor points of all cubics matching the anchor points of the "
            "preceding and succeeding cubics");
    }
    return feature;
}

std::vector<Cubic> transform_all(const std::vector<Cubic>& cubics, const PointTransformer& f) {
    std::vector<Cubic> result;
    result.reserve(cubics.size());
    for (const Cubic& c : cubics) {
        result.push_back(c.transformed(f));
    }
    return result;
}

std::vector<Cubic> reverse_all(const std::vector<Cubic>& cubics) {
    std::vector<Cubic> result;
    result.reserve(cubics.size());
    for (auto it = cubics.rbegin(); it != cubics.rend(); ++it) {
        result.push_back(it->reverse());
    }
    return result;
}

}

Feature::Feature(std::vector<Cubic> cubics, FeatureKind kind, bool convex)
    : cubics_(std::move(cubics)), kind_(kind), convex_(convex) {}

Feature Feature::edge(std::vector<Cubic> cubics) {
    return Feature(std::move(cubics), FeatureKind::edge, false);
}

Feature Feature::corner(std::vector<Cubic> cubics, bool convex) {
    return Feature(std::move(cubics), FeatureKind::corner, convex);
}

Feature Feature::build_ignorable_feature(std::vector<Cubic> cubics) {
    return validated(edge(std::move(cubics)));
}

Feature Feature::build_edge(const Cubic& cubic) {
    return edge({cubic});
}

Feature Feature::build_convex_corner(std::vector<Cubic> cubics) {
    return validated(corner(std::move(cubics), true));
}

Feature Feature::build_concave_corner(std::vector<Cubic> cubics) {
    return validated(corner(std::move(cubics), false));
}

Feature Feature::transformed(const PointTransformer& f) const {
    return Feature(transform_all(cubics_, f), kind_, convex_);
}

Feature Feature::reversed() const {
    if (kind_ == FeatureKind::corner) {
        return Feature(reverse_all(cubics_), kind_, !convex_);
    }
    return Feature(reverse_all(cubics_), kind_, convex_);
}

}
