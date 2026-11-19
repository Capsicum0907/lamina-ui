#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "lamina/motion/animated.hpp"
#include "lamina/render/geometry.hpp"

namespace lamina::motion {

class AnimatedRadii {
public:
    AnimatedRadii(const render::CornerRadii& value, SpringSpec spec)
        : corners_{Animated(value.top_left, spec), Animated(value.top_right, spec), Animated(value.bottom_right, spec),
                   Animated(value.bottom_left, spec)} {}

    void animate_to(const render::CornerRadii& target) {
        const auto v = as_array(target);
        for (std::size_t i = 0; i < corners_.size(); ++i) {
            corners_[i].animate_to(v[i]);
        }
    }

    void snap_to(const render::CornerRadii& value) {
        const auto v = as_array(value);
        for (std::size_t i = 0; i < corners_.size(); ++i) {
            corners_[i].snap_to(v[i]);
        }
    }

    void step(double dt_seconds) {
        for (Animated& c : corners_) {
            c.step(dt_seconds);
        }
    }

    render::CornerRadii value() const {
        return {std::max(0.0, corners_[0].value()), std::max(0.0, corners_[1].value()), std::max(0.0, corners_[2].value()),
                std::max(0.0, corners_[3].value())};
    }

    bool settled() const {
        return std::all_of(corners_.begin(), corners_.end(), [](const Animated& a) { return a.settled(); });
    }

private:
    static std::array<double, 4> as_array(const render::CornerRadii& r) {
        return {r.top_left, r.top_right, r.bottom_right, r.bottom_left};
    }

    std::array<Animated, 4> corners_;
};

}
