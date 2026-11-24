#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

#include "lamina/motion/animated.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/shape.hpp"

namespace lamina::widgets {

inline constexpr double center_optically_coefficient = 0.11;

class AnimatedCorners {
public:
    AnimatedCorners(const tokens::Corners& initial, motion::SpringSpec spec)
        : start_(linear(initial)), target_(linear(initial)), start_key_(initial), target_key_(initial), progress_(1.0, spec) {}

    const tokens::Corners& target() const { return target_key_; }

    void animate_to(const tokens::Corners& next) {
        if (next == target_key_) {
            return;
        }
        const double p = progress_.value();
        if (start_key_ && next == *start_key_) {
            std::swap(start_, target_);
            start_key_ = target_key_;
            target_key_ = next;
            progress_.restart(1.0 - p, -progress_.velocity(), 1.0);
            return;
        }
        Shape now{};
        for (std::size_t i = 0; i < now.size(); ++i) {
            now[i] = {motion::lerp(start_[i].dp, target_[i].dp, p), motion::lerp(start_[i].fraction, target_[i].fraction, p)};
        }
        start_key_ = p == 1.0 ? std::optional(target_key_) : p == 0.0 ? start_key_ : std::nullopt;
        start_ = now;
        target_ = linear(next);
        target_key_ = next;
        progress_.restart(0.0, 0.0, 1.0);
    }

    void snap_to(const tokens::Corners& c) {
        start_ = target_ = linear(c);
        start_key_ = target_key_ = c;
        progress_.snap_to(1.0);
    }

    void step(double dt_seconds) { progress_.step(dt_seconds); }
    bool settled() const { return progress_.settled(); }
    double progress() const { return progress_.value(); }

    render::CornerRadii radii(double width, double height) const {
        const double side = std::min(width, height);
        const double cap = std::max(0.0, side / 2.0);
        const double t = progress_.value();
        std::array<double, 4> r{};
        for (std::size_t i = 0; i < r.size(); ++i) {
            const double a = start_[i].dp + start_[i].fraction * side;
            const double b = target_[i].dp + target_[i].fraction * side;
            r[i] = std::clamp(motion::lerp(a, b, t), 0.0, cap);
        }
        return {r[0], r[1], r[2], r[3]};
    }

    double optical_offset(double width, double height) const {
        const render::CornerRadii r = radii(width, height);
        const double cap = height / 2.0;
        auto c = [cap](double v) { return std::clamp(v, 0.0, cap); };
        return center_optically_coefficient * ((c(r.top_left) + c(r.bottom_left)) / 2.0 - (c(r.top_right) + c(r.bottom_right)) / 2.0);
    }

private:
    struct CornerValue {
        double dp;
        double fraction;
    };
    using Shape = std::array<CornerValue, 4>;

    static CornerValue linear(tokens::CornerSize size) {
        if (size == tokens::CornerSize::full) {
            return {0.0, 0.5};
        }
        return {current_style().shape.radius(size, 0.0, 0.0), 0.0};
    }
    static Shape linear(const tokens::Corners& c) {
        return {linear(c.top_start), linear(c.top_end), linear(c.bottom_end), linear(c.bottom_start)};
    }

    Shape start_;
    Shape target_;
    std::optional<tokens::Corners> start_key_;
    tokens::Corners target_key_;
    motion::Animated progress_;
};

}
