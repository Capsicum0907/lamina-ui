#pragma once

#include <span>
#include <utility>
#include <vector>

namespace lamina::shapes::detail {

bool progress_in_range(float progress, float progress_from, float progress_to);

float linear_map(std::span<const float> x_values, std::span<const float> y_values, float x);

float progress_distance(float p1, float p2);

void validate_progress(std::span<const float> p);

class DoubleMapper {
public:
    explicit DoubleMapper(std::span<const std::pair<float, float>> mappings);

    float map(float x) const;
    float map_back(float x) const;

    static DoubleMapper identity();

private:
    std::vector<float> source_values_;
    std::vector<float> target_values_;
};

}
