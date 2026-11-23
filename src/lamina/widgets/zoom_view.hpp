#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lamina/input/events.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/motion/spline_decay.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/motion/zoom.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/zoom.hpp"
#include "lamina/widgets/accessibility.hpp"

namespace lamina::widgets {

struct ZoomViewOptions {
    std::string accessible_name;
    double content_width = 0.0;
    double content_height = 0.0;
    const tokens::ZoomTokens* tokens = &tokens::zoom;
};

class ZoomView {
public:
    explicit ZoomView(ZoomViewOptions options);

    void set_bounds(const render::Rect& bounds);
    const render::Rect& bounds() const { return bounds_; }
    void set_content_size(double width, double height);
    motion::ZoomGeometry geometry() const;
    const tokens::ZoomTokens& tokens() const { return *tokens_; }

    bool handle(const input::FingerEvent& event);
    bool handle(const input::PointerEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    bool holds_pointer() const;

    void step(double dt_seconds);
    bool animating() const;

    double scale() const { return shown_.scale; }
    const motion::ZoomTransform& transform() const { return shown_; }
    bool zoomed() const;
    bool at_min_scale() const { return !zoomed(); }
    motion::PanEdges edges() const;
    bool can_pan_horizontally(double dx) const;
    bool can_pan_vertically(double dy) const;
    bool gesture_active() const { return !fingers_.empty() || phase_ == Phase::mouse; }
    int fingers_down() const { return static_cast<int>(fingers_.size()); }

    void zoom_to(double scale, double x, double y);
    void reset();
    void snap_to_fit();

    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);

    std::optional<input::Cursor> cursor() const;
    std::optional<AccessibleNode> accessible() const;

    void begin_content(render::Canvas& canvas) const;
    void end_content(render::Canvas& canvas) const;

private:
    enum class Phase { idle, pending, panning, double_tap, handed, yielded, mouse };

    struct Finger {
        std::uint64_t id;
        double x;
        double y;
        double down_x;
        double down_y;
    };

    struct Tap {
        double x;
        double y;
        double up_seconds;
    };

    motion::ZoomLimits scale_limits() const { return {tokens_->min_scale, tokens_->max_scale}; }
    double focus_x() const;
    double focus_y() const;
    double span() const;
    void regroup();
    motion::ZoomTransform banded(const motion::ZoomTransform& raw, double fx, double fy) const;
    motion::ZoomTransform unbanded(const motion::ZoomTransform& shown, double fx, double fy) const;
    void catch_motion();
    void stop_motion();
    void settle_to(const motion::ZoomTransform& target);
    void settle_about(double fx, double fy);
    void release(double time_seconds, bool fling);
    void toggle_zoom(double fx, double fy);
    void yield();
    bool hands_off(double dx, double dy) const;
    bool finger_down(const input::FingerEvent& e);
    bool finger_move(const input::FingerEvent& e);
    bool finger_up(const input::FingerEvent& e);
    std::vector<Finger>::iterator find_finger(std::uint64_t id);

    const tokens::ZoomTokens* tokens_;
    std::string name_;
    double content_width_;
    double content_height_;
    render::Rect bounds_{0.0, 0.0, 0.0, 0.0};
    bool enabled_ = true;

    motion::ZoomTransform shown_{};
    motion::ZoomTransform raw_{};
    Phase phase_ = Phase::idle;
    bool scaling_ = false;
    std::vector<Finger> fingers_;
    double prev_focus_x_ = 0.0;
    double prev_focus_y_ = 0.0;
    double start_focus_x_ = 0.0;
    double start_focus_y_ = 0.0;
    double prev_span_ = 0.0;
    double initial_span_ = 0.0;
    bool single_tap_ = false;
    std::optional<Tap> last_tap_;
    bool after_touch_up_ = false;
    input::ScrollAxes waiting_{};
    double mouse_x_ = 0.0;
    double mouse_y_ = 0.0;
    motion::VelocityTracker tracker_x_;
    motion::VelocityTracker tracker_y_;

    bool settling_ = false;
    motion::ZoomTransform settle_from_{};
    motion::ZoomTransform settle_target_{};
    motion::SpringState settle_{0.0, 0.0};
    motion::Fling fling_x_;
    motion::Fling fling_y_;
};

}
