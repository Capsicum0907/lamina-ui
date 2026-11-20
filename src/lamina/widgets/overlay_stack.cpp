#include "lamina/widgets/overlay_stack.hpp"

#include <algorithm>
#include <concepts>

namespace lamina::widgets {

template <typename E>
bool OverlayStack::route(const E& e) {
    prune();
    bool stopped = false;
    ++routing_;
    for (std::size_t i = layers_.size(); i-- > 0;) {
        Layer* layer = layers_[i].get();
        if (!layer->live()) {
            continue;
        }
        const bool consumed = layer->deliver(e);
        if (consumed || layer->modality == Modality::modal) {
            stopped = true;
            break;
        }
    }
    --routing_;
    prune(!std::same_as<E, input::PointerEvent>);
    return stopped;
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

bool OverlayStack::handle(const input::WheelEvent& e) {
    return route(e);
}

bool OverlayStack::empty() const {
    return std::none_of(layers_.begin(), layers_.end(), [](const auto& layer) { return layer->live(); });
}

void OverlayStack::prune(bool restore_focus) {
    for (std::size_t i = layers_.size(); i-- > 0;) {
        if (layers_[i]->live()) {
            continue;
        }
        const bool give_back = layers_[i]->holds_focus;
        layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(i));
        if (give_back && focus_) {
            if (restore_focus) {
                focus_->resume();
            } else {
                focus_->discard();
            }
        }
    }
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
