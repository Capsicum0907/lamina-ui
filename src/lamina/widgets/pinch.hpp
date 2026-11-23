#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "lamina/input/events.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/tokens/zoom.hpp"

namespace lamina::widgets {

enum class PinchPhase { began, moved, ended };

struct PinchUpdate {
    PinchPhase phase;
    render::Point focus;
    double scale;
};

struct PinchOptions {
    double min_span;
    double span_slop;
};

constexpr PinchOptions pinch_options(const tokens::ZoomTokens& t) {
    return {t.min_span, t.span_slop};
}

class PinchRecognizer {
public:
    explicit PinchRecognizer(PinchOptions options = pinch_options(tokens::zoom));

    std::optional<PinchUpdate> handle(const input::FingerEvent& event);
    void cancel();

    bool active() const { return active_; }
    bool claimed() const { return claimed_; }
    int fingers() const { return static_cast<int>(fingers_.size()); }
    double scale() const { return scale_; }
    render::Point focus() const { return focus_; }

private:
    struct Finger {
        std::uint64_t id;
        double x;
        double y;
    };

    render::Point centroid() const;
    double span() const;
    void regroup();
    std::optional<PinchUpdate> lift(std::uint64_t id);

    PinchOptions options_;
    std::vector<Finger> fingers_;
    bool active_ = false;
    bool claimed_ = false;
    double initial_span_ = 0.0;
    double ref_span_ = 0.0;
    double base_scale_ = 1.0;
    double scale_ = 1.0;
    render::Point offset_{0.0, 0.0};
    render::Point focus_{0.0, 0.0};
};

}
