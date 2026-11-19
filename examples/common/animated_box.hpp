#pragma once

#include <algorithm>
#include <initializer_list>
#include <utility>

#include "lamina/motion/animated.hpp"
#include "lamina/render/geometry.hpp"

namespace lamina::examples {

class AnimatedBox {
public:
    explicit AnimatedBox(motion::SpringSpec spec) : x_(0.0, spec), y_(0.0, spec), w_(0.0, spec), h_(0.0, spec) {}

    template <typename Box>
    void move_to(const Box& b, bool animate) {
        const bool follow = animate || !settled();
        for (auto [a, v] : {std::pair{&x_, b.x}, std::pair{&y_, b.y}, std::pair{&w_, b.width}, std::pair{&h_, b.height}}) {
            if (follow) {
                a->animate_to(v);
            } else {
                a->snap_to(v);
            }
        }
    }
    template <typename Box>
    void snap_to(const Box& b) {
        x_.snap_to(b.x);
        y_.snap_to(b.y);
        w_.snap_to(b.width);
        h_.snap_to(b.height);
    }
    void step(double dt) {
        for (motion::Animated* a : {&x_, &y_, &w_, &h_}) {
            a->step(dt);
        }
    }
    bool settled() const { return x_.settled() && y_.settled() && w_.settled() && h_.settled(); }
    render::Rect rect() const { return {x_.value(), y_.value(), std::max(0.0, w_.value()), std::max(0.0, h_.value())}; }

private:
    motion::Animated x_;
    motion::Animated y_;
    motion::Animated w_;
    motion::Animated h_;
};

}
