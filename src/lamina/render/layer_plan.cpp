#include "lamina/render/layer_plan.hpp"

#include <algorithm>

namespace lamina::render {

bool overlaps(const Rect& a, const Rect& b) {
    return a.width > 0.0 && a.height > 0.0 && b.width > 0.0 && b.height > 0.0 && a.x < b.x + b.width && b.x < a.x + a.width &&
           a.y < b.y + b.height && b.y < a.y + a.height;
}

Rect united(const Rect& a, const Rect& b) {
    const double x0 = std::min(a.x, b.x);
    const double y0 = std::min(a.y, b.y);
    const double x1 = std::max(a.x + a.width, b.x + b.width);
    const double y1 = std::max(a.y + a.height, b.y + b.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

LayerPlan::LayerPlan(std::size_t max_layers) : max_layers_(std::max<std::size_t>(max_layers, 1)) {
    reset();
}

void LayerPlan::reset() {
    paint_bounds_.assign(1, std::nullopt);
    images_.clear();
}

std::size_t LayerPlan::place_paint(const Rect& device) {
    std::size_t layer = 0;
    for (const Placed& p : images_) {
        if (overlaps(p.bounds, device)) {
            layer = std::max(layer, p.batch + 1);
        }
    }
    for (std::size_t l = paint_bounds_.size(); l > layer + 1; --l) {
        const std::optional<Rect>& b = paint_bounds_[l - 1];
        if (b && overlaps(*b, device)) {
            layer = l - 1;
            break;
        }
    }
    if (layer >= paint_bounds_.size()) {
        paint_bounds_.resize(layer + 1);
    }
    if (device.width > 0.0 && device.height > 0.0) {
        std::optional<Rect>& b = paint_bounds_[layer];
        b = b ? united(*b, device) : device;
    }
    return layer;
}

std::optional<std::size_t> LayerPlan::place_image(const Rect& device) {
    std::size_t batch = 0;
    for (std::size_t l = paint_bounds_.size(); l > 0; --l) {
        const std::optional<Rect>& b = paint_bounds_[l - 1];
        if (b && overlaps(*b, device)) {
            batch = l - 1;
            break;
        }
    }
    for (const Placed& p : images_) {
        if (overlaps(p.bounds, device)) {
            batch = std::max(batch, p.batch);
        }
    }
    if (batch + 2 > max_layers_) {
        return std::nullopt;
    }
    images_.push_back({device, batch});
    return batch;
}

std::optional<Rect> LayerPlan::bounds(std::size_t layer) const {
    return layer < paint_bounds_.size() ? paint_bounds_[layer] : std::nullopt;
}

}
