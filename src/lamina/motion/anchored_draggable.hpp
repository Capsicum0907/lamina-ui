#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "lamina/motion/easing.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/motion/velocity_tracker.hpp"

namespace lamina::motion {

using OffsetMotion = std::variant<SpringSpec, TweenSpec>;

struct DragThresholds {
    std::function<double(double distance)> positional;
    double velocity;
};

struct DragGesture {
    double slop;
    VelocityTrackerSpec tracking;
    double maximum_velocity;
};

template <typename T>
class AnchoredDraggable {
public:
    struct Anchor {
        T value;
        double position;
    };

    AnchoredDraggable(T initial, DragThresholds thresholds, DragGesture gesture)
        : settled_(initial), target_(initial), thresholds_(std::move(thresholds)), gesture_(gesture), tracker_(gesture.tracking) {}

    void set_anchors(std::vector<Anchor> anchors, T target) {
        anchors_ = std::move(anchors);
        std::sort(anchors_.begin(), anchors_.end(), [](const Anchor& a, const Anchor& b) { return a.position < b.position; });
        const std::optional<double> p = position_of(target);
        if (!p) {
            return;
        }
        if (!positioned_) {
            positioned_ = true;
            offset_ = *p;
            settled_ = target_ = target;
            return;
        }
        if (dragging_) {
            offset_ = clamp(offset_);
            return;
        }
        if (animating_) {
            if (target != target_ || *p != destination_) {
                start(target, motion_, velocity_);
            }
            return;
        }
        settled_ = target_ = target;
        offset_ = *p;
    }

    void set_gesture(DragGesture gesture) { gesture_ = gesture; }
    void retune(const SpringSpec& spring) {
        if (animating_ && std::holds_alternative<SpringSpec>(motion_)) {
            motion_ = spring;
        }
    }

    bool positioned() const { return positioned_; }
    bool has(T value) const { return position_of(value).has_value(); }
    std::optional<double> position_of(T value) const {
        for (const Anchor& a : anchors_) {
            if (a.value == value) {
                return a.position;
            }
        }
        return std::nullopt;
    }
    const std::vector<Anchor>& anchors() const { return anchors_; }
    double min_position() const { return anchors_.empty() ? 0.0 : anchors_.front().position; }
    double max_position() const { return anchors_.empty() ? 0.0 : anchors_.back().position; }

    double offset() const { return offset_; }
    double velocity() const { return velocity_; }
    double last_velocity() const { return last_velocity_; }
    T settled_value() const { return settled_; }
    T target_value() const { return target_; }
    bool pressed() const { return pressed_; }
    bool dragging() const { return dragging_; }
    bool animating() const { return animating_; }
    bool idle() const { return !animating_ && !dragging_; }

    void animate_to(T value, const OffsetMotion& motion) { animate_to(value, motion, last_velocity_); }
    void animate_to(T value, const OffsetMotion& motion, double velocity) {
        if (!has(value)) {
            return;
        }
        dragging_ = false;
        pressed_ = false;
        start(value, motion, velocity);
    }

    void snap_to(T value) {
        const std::optional<double> p = position_of(value);
        if (!p) {
            return;
        }
        animating_ = false;
        dragging_ = false;
        pressed_ = false;
        offset_ = *p;
        velocity_ = 0.0;
        settled_ = target_ = value;
    }

    void press(double pointer, double time_seconds) {
        pressed_ = true;
        dragging_ = false;
        press_pointer_ = pointer;
        tracker_ = VelocityTracker(gesture_.tracking);
        tracker_.add(time_seconds, pointer);
    }

    bool move(double pointer, double time_seconds) {
        if (!pressed_) {
            return false;
        }
        tracker_.add(time_seconds, pointer);
        if (!dragging_) {
            const double d = pointer - press_pointer_;
            if (std::abs(d) <= gesture_.slop) {
                return false;
            }
            dragging_ = true;
            animating_ = false;
            velocity_ = 0.0;
            drag_offset_ = offset_;
            drag_pointer_ = press_pointer_ + (d > 0.0 ? gesture_.slop : -gesture_.slop);
        }
        offset_ = clamp(drag_offset_ + pointer - drag_pointer_);
        target_ = closest(offset_);
        return true;
    }

    std::optional<double> release(double time_seconds) {
        pressed_ = false;
        if (!dragging_) {
            return std::nullopt;
        }
        dragging_ = false;
        tracker_.release(time_seconds);
        last_velocity_ = tracker_.velocity(gesture_.maximum_velocity);
        return last_velocity_;
    }

    void cancel_press() {
        pressed_ = false;
        dragging_ = false;
    }

    T fling(double velocity, const OffsetMotion& motion) {
        const T target = compute_target(offset_, velocity);
        last_velocity_ = velocity;
        start(target, motion, velocity);
        return target;
    }

    T compute_target(double offset, double velocity) const {
        const bool moving = std::abs(velocity) > 0.0;
        const bool forward = moving && velocity > 0.0;
        if (!moving) {
            return closest(offset);
        }
        if (std::abs(velocity) >= std::abs(thresholds_.velocity)) {
            return closest(offset, forward);
        }
        const Anchor& left = closest_anchor(offset, false);
        const Anchor& right = closest_anchor(offset, true);
        const double distance = std::abs(left.position - right.position);
        const double threshold = std::abs(thresholds_.positional(distance));
        const double from = forward ? left.position : right.position;
        const bool crossed = std::abs(from - offset) >= threshold;
        if (crossed) {
            return forward ? right.value : left.value;
        }
        return forward ? left.value : right.value;
    }

    T closest(double offset) const {
        const Anchor* best = &anchors_.front();
        for (const Anchor& a : anchors_) {
            if (std::abs(a.position - offset) < std::abs(best->position - offset)) {
                best = &a;
            }
        }
        return best->value;
    }
    T closest(double offset, bool upwards) const { return closest_anchor(offset, upwards).value; }

    void step(double dt_seconds) {
        if (!animating_) {
            return;
        }
        if (const auto* spring = std::get_if<SpringSpec>(&motion_)) {
            const SpringState s = advance(*spring, {offset_, velocity_}, destination_, dt_seconds);
            offset_ = s.value;
            velocity_ = s.velocity;
            if (is_settled(s, destination_)) {
                finish();
            }
            return;
        }
        const TweenSpec& tween = std::get<TweenSpec>(motion_);
        elapsed_ += dt_seconds;
        const double f = tween.duration_seconds <= 0.0 ? 1.0 : std::min(1.0, elapsed_ / tween.duration_seconds);
        const double previous = offset_;
        offset_ = lerp(from_, destination_, ease(tween.easing, f));
        velocity_ = dt_seconds > 0.0 ? (offset_ - previous) / dt_seconds : 0.0;
        if (f >= 1.0) {
            finish();
        }
    }

private:
    double clamp(double offset) const { return std::clamp(offset, min_position(), max_position()); }

    const Anchor& closest_anchor(double offset, bool upwards) const {
        const Anchor* best = &anchors_.front();
        double best_distance = std::numeric_limits<double>::infinity();
        for (const Anchor& a : anchors_) {
            const double delta = upwards ? a.position - offset : offset - a.position;
            const double distance = delta < 0.0 ? std::numeric_limits<double>::infinity() : delta;
            if (distance <= best_distance) {
                best = &a;
                best_distance = distance;
            }
        }
        return *best;
    }

    void start(T target, const OffsetMotion& motion, double velocity) {
        const std::optional<double> p = position_of(target);
        if (!p) {
            return;
        }
        target_ = target;
        motion_ = motion;
        destination_ = *p;
        from_ = offset_;
        elapsed_ = 0.0;
        velocity_ = std::holds_alternative<SpringSpec>(motion) ? velocity : 0.0;
        animating_ = offset_ != destination_ || velocity_ != 0.0;
        if (!animating_) {
            settled_ = target;
        }
    }

    void finish() {
        offset_ = destination_;
        velocity_ = 0.0;
        animating_ = false;
        settled_ = target_;
    }

    std::vector<Anchor> anchors_;
    T settled_;
    T target_;
    DragThresholds thresholds_;
    DragGesture gesture_;
    VelocityTracker tracker_;
    OffsetMotion motion_{};
    bool positioned_ = false;
    bool animating_ = false;
    bool pressed_ = false;
    bool dragging_ = false;
    double offset_ = 0.0;
    double velocity_ = 0.0;
    double last_velocity_ = 0.0;
    double destination_ = 0.0;
    double from_ = 0.0;
    double elapsed_ = 0.0;
    double press_pointer_ = 0.0;
    double drag_offset_ = 0.0;
    double drag_pointer_ = 0.0;
};

}
