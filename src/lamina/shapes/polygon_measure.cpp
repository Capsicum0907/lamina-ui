#include "lamina/shapes/polygon_measure.hpp"

#include <limits>
#include <stdexcept>

namespace lamina::shapes::detail {

namespace {

constexpr int length_measurer_segments = 3;

std::pair<float, float> closest_progress_to(const Cubic& cubic, float threshold) {
    float total = 0.0f;
    float remainder = threshold;
    Point prev = cubic.anchor0();
    const auto segments = static_cast<float>(length_measurer_segments);
    for (int i = 1; i <= length_measurer_segments; ++i) {
        const float progress = static_cast<float>(i) / segments;
        const Point point = cubic.point_on_curve(progress);
        const float segment = get_distance(point - prev);
        if (segment >= remainder) {
            return {progress - (1.0f - remainder / segment) / segments, threshold};
        }
        remainder -= segment;
        total += segment;
        prev = point;
    }
    return {1.0f, total};
}

float coerce_in(float value, float lo, float hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

}

float measure_cubic(const Cubic& c) {
    return closest_progress_to(c, std::numeric_limits<float>::infinity()).second;
}

float find_cubic_cut_point(const Cubic& c, float m) {
    return closest_progress_to(c, m).first;
}

MeasuredCubic::MeasuredCubic(const Cubic& cubic, float start_outline_progress, float end_outline_progress)
    : cubic_(cubic),
      measured_size_(measure_cubic(cubic)),
      start_outline_progress_(start_outline_progress),
      end_outline_progress_(end_outline_progress) {
    if (!(end_outline_progress >= start_outline_progress)) {
        throw std::invalid_argument("endOutlineProgress is expected to be equal or greater than startOutlineProgress");
    }
}

void MeasuredCubic::update_progress_range(float start_outline_progress, float end_outline_progress) {
    if (!(end_outline_progress >= start_outline_progress)) {
        throw std::invalid_argument("endOutlineProgress is expected to be equal or greater than startOutlineProgress");
    }
    start_outline_progress_ = start_outline_progress;
    end_outline_progress_ = end_outline_progress;
}

std::pair<MeasuredCubic, MeasuredCubic> MeasuredCubic::cut_at_progress(float cut_outline_progress) const {
    const float bounded = coerce_in(cut_outline_progress, start_outline_progress_, end_outline_progress_);
    const float outline_progress_size = end_outline_progress_ - start_outline_progress_;
    const float progress_from_start = bounded - start_outline_progress_;
    const float relative_progress = progress_from_start / outline_progress_size;
    const float t = find_cubic_cut_point(cubic_, relative_progress * measured_size_);
    if (!(t >= 0.0f && t <= 1.0f)) {
        throw std::logic_error("Cubic cut point is expected to be between 0 and 1");
    }
    const auto [c1, c2] = cubic_.split(t);
    return {MeasuredCubic(c1, start_outline_progress_, bounded), MeasuredCubic(c2, bounded, end_outline_progress_)};
}

MeasuredPolygon::MeasuredPolygon(std::vector<ProgressableFeature> features, const std::vector<Cubic>& cubics,
                                 const std::vector<float>& outline_progress)
    : features_(std::move(features)) {
    if (outline_progress.size() != cubics.size() + 1) {
        throw std::invalid_argument("Outline progress size is expected to be the cubics size + 1");
    }
    if (outline_progress.front() != 0.0f) {
        throw std::invalid_argument("First outline progress value is expected to be zero");
    }
    if (outline_progress.back() != 1.0f) {
        throw std::invalid_argument("Last outline progress value is expected to be one");
    }
    float start_outline_progress = 0.0f;
    for (std::size_t index = 0; index < cubics.size(); ++index) {
        if ((outline_progress[index + 1] - outline_progress[index]) > distance_epsilon) {
            cubics_.emplace_back(cubics[index], start_outline_progress, outline_progress[index + 1]);
            start_outline_progress = outline_progress[index + 1];
        }
    }
    if (cubics_.empty()) {
        throw std::logic_error("MeasuredPolygon has no non-empty cubics");
    }
    cubics_.back().update_progress_range(cubics_.back().start_outline_progress(), 1.0f);
}

MeasuredPolygon MeasuredPolygon::cut_and_shift(float cutting_point) const {
    if (!(cutting_point >= 0.0f && cutting_point <= 1.0f)) {
        throw std::invalid_argument("Cutting point is expected to be between 0 and 1");
    }
    if (cutting_point < distance_epsilon) {
        return *this;
    }
    std::size_t target_index = cubics_.size();
    for (std::size_t i = 0; i < cubics_.size(); ++i) {
        if (cutting_point >= cubics_[i].start_outline_progress() && cutting_point <= cubics_[i].end_outline_progress()) {
            target_index = i;
            break;
        }
    }
    if (target_index == cubics_.size()) {
        throw std::logic_error("Cutting point is not inside any cubic");
    }
    const auto [b1, b2] = cubics_[target_index].cut_at_progress(cutting_point);

    const std::size_t n = cubics_.size();
    std::vector<Cubic> ret_cubics;
    ret_cubics.reserve(n + 1);
    ret_cubics.push_back(b2.cubic());
    for (std::size_t i = 1; i < n; ++i) {
        ret_cubics.push_back(cubics_[(i + target_index) % n].cubic());
    }
    ret_cubics.push_back(b1.cubic());

    std::vector<float> ret_outline_progress;
    ret_outline_progress.reserve(n + 2);
    for (std::size_t index = 0; index < n + 2; ++index) {
        if (index == 0) {
            ret_outline_progress.push_back(0.0f);
        } else if (index == n + 1) {
            ret_outline_progress.push_back(1.0f);
        } else {
            const std::size_t cubic_index = (target_index + index - 1) % n;
            ret_outline_progress.push_back(
                positive_modulo(cubics_[cubic_index].end_outline_progress() - cutting_point, 1.0f));
        }
    }

    std::vector<ProgressableFeature> new_features;
    new_features.reserve(features_.size());
    for (const ProgressableFeature& f : features_) {
        new_features.push_back({positive_modulo(f.progress - cutting_point, 1.0f), f.feature});
    }
    return MeasuredPolygon(std::move(new_features), ret_cubics, ret_outline_progress);
}

MeasuredPolygon MeasuredPolygon::measure_polygon(const RoundedPolygon& polygon) {
    std::vector<Cubic> cubics;
    std::vector<std::pair<const Feature*, std::size_t>> feature_to_cubic;
    for (const Feature& feature : polygon.features()) {
        const auto& feature_cubics = feature.cubics();
        for (std::size_t cubic_index = 0; cubic_index < feature_cubics.size(); ++cubic_index) {
            if (feature.kind() == FeatureKind::corner && cubic_index == feature_cubics.size() / 2) {
                feature_to_cubic.emplace_back(&feature, cubics.size());
            }
            cubics.push_back(feature_cubics[cubic_index]);
        }
    }

    std::vector<float> measures;
    measures.reserve(cubics.size() + 1);
    measures.push_back(0.0f);
    for (const Cubic& cubic : cubics) {
        const float m = measure_cubic(cubic);
        if (!(m >= 0.0f)) {
            throw std::logic_error("Measured cubic is expected to be greater or equal to zero");
        }
        measures.push_back(measures.back() + m);
    }
    const float total_measure = measures.back();

    std::vector<float> outline_progress;
    outline_progress.reserve(measures.size());
    for (const float m : measures) {
        outline_progress.push_back(m / total_measure);
    }

    std::vector<ProgressableFeature> features;
    features.reserve(feature_to_cubic.size());
    for (const auto& [feature, ix] : feature_to_cubic) {
        features.push_back(
            {positive_modulo((outline_progress[ix] + outline_progress[ix + 1]) / 2.0f, 1.0f), feature});
    }
    return MeasuredPolygon(std::move(features), cubics, outline_progress);
}

}
