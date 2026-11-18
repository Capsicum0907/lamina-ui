#include "lamina/shapes/float_mapping.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include "lamina/shapes/point.hpp"

namespace lamina::shapes::detail {

namespace {

constexpr float min_segment_size = 0.001f;
constexpr float degenerate_segment_position = 0.5f;
constexpr float identity_second_point = 0.5f;

}

bool progress_in_range(float progress, float progress_from, float progress_to) {
    if (progress_to >= progress_from) {
        return progress >= progress_from && progress <= progress_to;
    }
    return progress >= progress_from || progress <= progress_to;
}

float linear_map(std::span<const float> x_values, std::span<const float> y_values, float x) {
    if (!(x >= 0.0f && x <= 1.0f)) {
        throw std::invalid_argument("Invalid progress");
    }
    const std::size_t size = x_values.size();
    std::size_t segment_start_index = size;
    for (std::size_t i = 0; i < size; ++i) {
        if (progress_in_range(x, x_values[i], x_values[(i + 1) % size])) {
            segment_start_index = i;
            break;
        }
    }
    if (segment_start_index == size) {
        throw std::logic_error("Collection contains no element matching the predicate.");
    }
    const std::size_t segment_end_index = (segment_start_index + 1) % size;
    const float segment_size_x = positive_modulo(x_values[segment_end_index] - x_values[segment_start_index], 1.0f);
    const float segment_size_y = positive_modulo(y_values[segment_end_index] - y_values[segment_start_index], 1.0f);
    const float position_in_segment =
        segment_size_x < min_segment_size
            ? degenerate_segment_position
            : positive_modulo(x - x_values[segment_start_index], 1.0f) / segment_size_x;
    return positive_modulo(y_values[segment_start_index] + segment_size_y * position_in_segment, 1.0f);
}

float progress_distance(float p1, float p2) {
    const float d = std::abs(p1 - p2);
    return std::min(d, 1.0f - d);
}

void validate_progress(std::span<const float> p) {
    float prev = p.back();
    int wraps = 0;
    for (const float curr : p) {
        if (!(curr >= 0.0f && curr < 1.0f)) {
            throw std::invalid_argument("FloatMapping - Progress outside of range");
        }
        if (!(progress_distance(curr, prev) > distance_epsilon)) {
            throw std::invalid_argument("FloatMapping - Progress repeats a value");
        }
        if (curr < prev) {
            ++wraps;
            if (wraps > 1) {
                throw std::invalid_argument("FloatMapping - Progress wraps more than once");
            }
        }
        prev = curr;
    }
}

DoubleMapper::DoubleMapper(std::span<const std::pair<float, float>> mappings) {
    source_values_.reserve(mappings.size());
    target_values_.reserve(mappings.size());
    for (const auto& [source, target] : mappings) {
        source_values_.push_back(source);
        target_values_.push_back(target);
    }
    validate_progress(source_values_);
    validate_progress(target_values_);
}

float DoubleMapper::map(float x) const {
    return linear_map(source_values_, target_values_, x);
}

float DoubleMapper::map_back(float x) const {
    return linear_map(target_values_, source_values_, x);
}

DoubleMapper DoubleMapper::identity() {
    const std::array<std::pair<float, float>, 2> mappings{
        std::pair{0.0f, 0.0f},
        std::pair{identity_second_point, identity_second_point},
    };
    return DoubleMapper(mappings);
}

}
