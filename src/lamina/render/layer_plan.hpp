#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::render {

class LayerPlan {
public:
    explicit LayerPlan(std::size_t max_layers);

    void reset();
    std::size_t place_paint(const Rect& device);
    std::optional<std::size_t> place_image(const Rect& device);

    std::optional<Rect> bounds(std::size_t layer) const;
    std::size_t layers() const { return paint_bounds_.size(); }
    std::size_t max_layers() const { return max_layers_; }

private:
    struct Placed {
        Rect bounds;
        std::size_t batch;
    };

    std::size_t max_layers_;
    std::vector<std::optional<Rect>> paint_bounds_;
    std::vector<Placed> images_;
};

bool overlaps(const Rect& a, const Rect& b);
Rect united(const Rect& a, const Rect& b);

}
