#pragma once

#include <utility>
#include <vector>

#include "lamina/shapes/float_mapping.hpp"
#include "lamina/shapes/polygon_measure.hpp"

namespace lamina::shapes::detail {

DoubleMapper feature_mapper(const std::vector<ProgressableFeature>& features1,
                            const std::vector<ProgressableFeature>& features2);

std::vector<std::pair<float, float>> do_mapping(const std::vector<ProgressableFeature>& features1,
                                                const std::vector<ProgressableFeature>& features2);

float feature_dist_squared(const Feature& f1, const Feature& f2);

Point feature_representative_point(const Feature& feature);

}
