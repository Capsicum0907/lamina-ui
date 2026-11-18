#pragma once

#include <utility>
#include <vector>

#include "lamina/shapes/rounded_polygon.hpp"

namespace lamina::shapes {

class Morph {
public:
    Morph(RoundedPolygon start, RoundedPolygon end);

    Bounds calculate_bounds(bool approximate = true) const;
    Bounds calculate_max_bounds() const;

    std::vector<Cubic> as_cubics(float progress) const;

    template <typename Callback>
    void for_each_cubic(float progress, Callback&& callback) const {
        for (const auto& [first, second] : morph_match_) {
            const Cubic cubic = Cubic::interpolate(first, second, progress);
            callback(cubic);
        }
    }

    const std::vector<std::pair<Cubic, Cubic>>& morph_match() const { return morph_match_; }

    static std::vector<std::pair<Cubic, Cubic>> match(const RoundedPolygon& p1, const RoundedPolygon& p2);

private:
    RoundedPolygon start_;
    RoundedPolygon end_;
    std::vector<std::pair<Cubic, Cubic>> morph_match_;
};

}
