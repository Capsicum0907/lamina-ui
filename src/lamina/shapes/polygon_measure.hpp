#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "lamina/shapes/rounded_polygon.hpp"

namespace lamina::shapes::detail {

struct ProgressableFeature {
    float progress;
    const Feature* feature;
};

float measure_cubic(const Cubic& c);
float find_cubic_cut_point(const Cubic& c, float m);

class MeasuredCubic {
public:
    MeasuredCubic(const Cubic& cubic, float start_outline_progress, float end_outline_progress);

    const Cubic& cubic() const { return cubic_; }
    float measured_size() const { return measured_size_; }
    float start_outline_progress() const { return start_outline_progress_; }
    float end_outline_progress() const { return end_outline_progress_; }

    void update_progress_range(float start_outline_progress, float end_outline_progress);
    std::pair<MeasuredCubic, MeasuredCubic> cut_at_progress(float cut_outline_progress) const;

private:
    Cubic cubic_;
    float measured_size_;
    float start_outline_progress_;
    float end_outline_progress_;
};

class MeasuredPolygon {
public:
    static MeasuredPolygon measure_polygon(const RoundedPolygon& polygon);

    MeasuredPolygon cut_and_shift(float cutting_point) const;

    const std::vector<ProgressableFeature>& features() const { return features_; }
    std::size_t size() const { return cubics_.size(); }
    const MeasuredCubic& operator[](std::size_t index) const { return cubics_[index]; }

private:
    MeasuredPolygon(std::vector<ProgressableFeature> features, const std::vector<Cubic>& cubics,
                    const std::vector<float>& outline_progress);

    std::vector<MeasuredCubic> cubics_;
    std::vector<ProgressableFeature> features_;
};

}
