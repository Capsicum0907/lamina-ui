#include "lamina/shapes/feature_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace lamina::shapes::detail {

namespace {

constexpr float half_progress = 0.5f;

struct DistanceVertex {
    float distance;
    std::size_t f1;
    std::size_t f2;
};

class MappingHelper {
public:
    MappingHelper(std::size_t size1, std::size_t size2) : used_f1_(size1, false), used_f2_(size2, false) {}

    void add_mapping(std::size_t i1, const ProgressableFeature& f1, std::size_t i2, const ProgressableFeature& f2) {
        if (used_f1_[i1] || used_f2_[i2]) {
            return;
        }
        const auto it = std::lower_bound(mapping_.begin(), mapping_.end(), f1.progress,
                                         [](const std::pair<float, float>& m, float v) { return m.first < v; });
        if (it != mapping_.end() && it->first == f1.progress) {
            throw std::invalid_argument("There can't be two features with the same progress");
        }
        const auto insertion_index = static_cast<std::size_t>(it - mapping_.begin());
        const std::size_t n = mapping_.size();
        if (n >= 1) {
            const auto [before1, before2] = mapping_[(insertion_index + n - 1) % n];
            const auto [after1, after2] = mapping_[insertion_index % n];
            if (progress_distance(f1.progress, before1) < distance_epsilon ||
                progress_distance(f1.progress, after1) < distance_epsilon ||
                progress_distance(f2.progress, before2) < distance_epsilon ||
                progress_distance(f2.progress, after2) < distance_epsilon) {
                return;
            }
            if (n > 1 && !progress_in_range(f2.progress, before2, after2)) {
                return;
            }
        }
        mapping_.insert(mapping_.begin() + static_cast<std::ptrdiff_t>(insertion_index), {f1.progress, f2.progress});
        used_f1_[i1] = true;
        used_f2_[i2] = true;
    }

    std::vector<std::pair<float, float>> take_mapping() { return std::move(mapping_); }

private:
    std::vector<std::pair<float, float>> mapping_;
    std::vector<bool> used_f1_;
    std::vector<bool> used_f2_;
};

std::vector<ProgressableFeature> corners_only(const std::vector<ProgressableFeature>& features) {
    std::vector<ProgressableFeature> result;
    for (const ProgressableFeature& f : features) {
        if (f.feature->kind() == FeatureKind::corner) {
            result.push_back(f);
        }
    }
    return result;
}

}

DoubleMapper feature_mapper(const std::vector<ProgressableFeature>& features1,
                            const std::vector<ProgressableFeature>& features2) {
    const auto mapping = do_mapping(corners_only(features1), corners_only(features2));
    return DoubleMapper(mapping);
}

std::vector<std::pair<float, float>> do_mapping(const std::vector<ProgressableFeature>& features1,
                                                const std::vector<ProgressableFeature>& features2) {
    std::vector<DistanceVertex> distance_vertex_list;
    for (std::size_t i1 = 0; i1 < features1.size(); ++i1) {
        for (std::size_t i2 = 0; i2 < features2.size(); ++i2) {
            const float d = feature_dist_squared(*features1[i1].feature, *features2[i2].feature);
            if (d != std::numeric_limits<float>::max()) {
                distance_vertex_list.push_back({d, i1, i2});
            }
        }
    }
    std::stable_sort(distance_vertex_list.begin(), distance_vertex_list.end(),
                     [](const DistanceVertex& a, const DistanceVertex& b) { return a.distance < b.distance; });

    if (distance_vertex_list.empty()) {
        return {{0.0f, 0.0f}, {half_progress, half_progress}};
    }
    if (distance_vertex_list.size() == 1) {
        const float f1 = features1[distance_vertex_list.front().f1].progress;
        const float f2 = features2[distance_vertex_list.front().f2].progress;
        return {{f1, f2}, {std::fmod(f1 + half_progress, 1.0f), std::fmod(f2 + half_progress, 1.0f)}};
    }

    MappingHelper helper(features1.size(), features2.size());
    for (const DistanceVertex& dv : distance_vertex_list) {
        helper.add_mapping(dv.f1, features1[dv.f1], dv.f2, features2[dv.f2]);
    }
    return helper.take_mapping();
}

float feature_dist_squared(const Feature& f1, const Feature& f2) {
    if (f1.kind() == FeatureKind::corner && f2.kind() == FeatureKind::corner && f1.convex() != f2.convex()) {
        return std::numeric_limits<float>::max();
    }
    return get_distance_squared(feature_representative_point(f1) - feature_representative_point(f2));
}

Point feature_representative_point(const Feature& feature) {
    const float x = (feature.cubics().front().anchor0_x() + feature.cubics().back().anchor1_x()) / 2.0f;
    const float y = (feature.cubics().front().anchor0_y() + feature.cubics().back().anchor1_y()) / 2.0f;
    return {x, y};
}

}
