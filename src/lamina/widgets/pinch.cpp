#include "lamina/widgets/pinch.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::widgets {

PinchRecognizer::PinchRecognizer(PinchOptions options) : options_(options) {}

render::Point PinchRecognizer::centroid() const {
    if (fingers_.empty()) {
        return {0.0, 0.0};
    }
    double x = 0.0;
    double y = 0.0;
    for (const Finger& f : fingers_) {
        x += f.x;
        y += f.y;
    }
    const auto n = static_cast<double>(fingers_.size());
    return {x / n, y / n};
}

double PinchRecognizer::span() const {
    if (fingers_.size() < 2) {
        return 0.0;
    }
    const render::Point c = centroid();
    double dev_x = 0.0;
    double dev_y = 0.0;
    for (const Finger& f : fingers_) {
        dev_x += std::abs(f.x - c.x);
        dev_y += std::abs(f.y - c.y);
    }
    const auto n = static_cast<double>(fingers_.size());
    return std::hypot(dev_x / n * 2.0, dev_y / n * 2.0);
}

void PinchRecognizer::regroup() {
    if (active_) {
        base_scale_ = scale_;
        ref_span_ = span();
        const render::Point c = centroid();
        offset_ = {focus_.x - c.x, focus_.y - c.y};
        return;
    }
    initial_span_ = span();
}

std::optional<PinchUpdate> PinchRecognizer::lift(std::uint64_t id) {
    const auto it = std::find_if(fingers_.begin(), fingers_.end(), [id](const Finger& f) { return f.id == id; });
    if (it == fingers_.end()) {
        return std::nullopt;
    }
    fingers_.erase(it);
    if (fingers_.empty()) {
        claimed_ = false;
    }
    if (active_ && fingers_.size() < 2) {
        active_ = false;
        return PinchUpdate{PinchPhase::ended, focus_, scale_};
    }
    regroup();
    return std::nullopt;
}

std::optional<PinchUpdate> PinchRecognizer::handle(const input::FingerEvent& e) {
    switch (e.action) {
    case input::FingerAction::down: {
        std::optional<PinchUpdate> ended;
        if (std::any_of(fingers_.begin(), fingers_.end(), [&e](const Finger& f) { return f.id == e.finger; })) {
            ended = lift(e.finger);
        }
        fingers_.push_back({e.finger, e.x_dp, e.y_dp});
        if (fingers_.size() >= 2) {
            claimed_ = true;
        }
        regroup();
        return ended;
    }
    case input::FingerAction::move: {
        const auto it = std::find_if(fingers_.begin(), fingers_.end(), [&e](const Finger& f) { return f.id == e.finger; });
        if (it == fingers_.end()) {
            return std::nullopt;
        }
        it->x = e.x_dp;
        it->y = e.y_dp;
        if (fingers_.size() < 2) {
            return std::nullopt;
        }
        const double sp = span();
        const render::Point c = centroid();
        if (!active_) {
            if (sp < options_.min_span || std::abs(sp - initial_span_) <= options_.span_slop) {
                return std::nullopt;
            }
            active_ = true;
            ref_span_ = sp;
            base_scale_ = 1.0;
            scale_ = 1.0;
            offset_ = {0.0, 0.0};
            focus_ = c;
            return PinchUpdate{PinchPhase::began, focus_, scale_};
        }
        scale_ = ref_span_ > 0.0 ? base_scale_ * sp / ref_span_ : base_scale_;
        focus_ = {c.x + offset_.x, c.y + offset_.y};
        return PinchUpdate{PinchPhase::moved, focus_, scale_};
    }
    case input::FingerAction::up:
    case input::FingerAction::cancel: return lift(e.finger);
    }
    return std::nullopt;
}

void PinchRecognizer::cancel() {
    fingers_.clear();
    active_ = false;
    claimed_ = false;
    base_scale_ = 1.0;
    scale_ = 1.0;
    offset_ = {0.0, 0.0};
}

}
