#include "lamina/widgets/overlay_stack.hpp"

#include <algorithm>

namespace lamina::widgets {

template <typename E>
bool OverlayStack::route(const E& e) {
    prune();
    for (std::size_t i = layers_.size(); i-- > 0;) {
        Layer* layer = layers_[i].get();
        if (!layer->live()) {
            continue;
        }
        const bool consumed = layer->deliver(e);
        if (consumed || layer->modality == Modality::modal) {
            return true;
        }
    }
    return false;
}

bool OverlayStack::handle(const input::PointerEvent& e) {
    return route(e);
}

bool OverlayStack::handle(const input::KeyEvent& e) {
    return route(e);
}

bool OverlayStack::handle(const input::TextEvent& e) {
    return route(e);
}

bool OverlayStack::empty() const {
    return std::none_of(layers_.begin(), layers_.end(), [](const auto& layer) { return layer->live(); });
}

void OverlayStack::prune() {
    std::erase_if(layers_, [](const auto& layer) { return !layer->live(); });
}

void OverlayStack::cancel_below() {
    for (auto& layer : layers_) {
        if (layer->live()) {
            layer->cancel_interaction();
        }
    }
    for (auto& cancel : base_) {
        cancel();
    }
}

}
