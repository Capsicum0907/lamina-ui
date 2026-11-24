#pragma once

#include <algorithm>
#include <cmath>

#include "lamina/motion/animated.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/top_app_bar.hpp"

namespace lamina::widgets {

enum class TopAppBarScroll { pinned, enter_always, exit_until_collapsed };

class TopAppBarState {
public:
    explicit TopAppBarState(TopAppBarScroll behavior = TopAppBarScroll::pinned)
        : behavior_(behavior), settle_(0.0, current_style().top_app_bar.common.snap_spring) {}

    TopAppBarScroll behavior() const { return behavior_; }
    double height_offset_limit() const { return limit_; }
    void set_height_offset_limit(double limit) {
        limit_ = std::min(0.0, limit);
        height_offset_ = std::clamp(height_offset_, limit_, 0.0);
    }
    double height_offset() const { return height_offset_; }
    double content_offset() const { return content_offset_; }
    double content_scroll() const { return content_; }
    double scroll_offset() const { return content_ - height_offset_; }

    double collapsed_fraction() const { return limit_ != 0.0 ? height_offset_ / limit_ : 0.0; }
    double overlapped_fraction() const {
        if (content_ > 0.0 && content_offset_ == 0.0) {
            return 1.0;
        }
        return limit_ != 0.0 ? std::min(std::abs(content_offset_) / std::abs(limit_), 1.0) : 0.0;
    }

    void scrolled_to(double offset) {
        if (settling_) {
            return;
        }
        const double delta = offset - last_offset_;
        last_offset_ = offset;
        const double before = content_;
        double h = height_offset_;
        switch (behavior_) {
        case TopAppBarScroll::pinned:
            h = 0.0;
            break;
        case TopAppBarScroll::enter_always:
            h = std::clamp(h - delta, limit_, 0.0);
            break;
        case TopAppBarScroll::exit_until_collapsed:
            if (delta > 0.0) {
                h = std::clamp(h - delta, limit_, 0.0);
            } else {
                const double content = std::max(0.0, before + delta);
                h = std::clamp(content - offset, limit_, 0.0);
            }
            break;
        }
        height_offset_ = std::clamp(std::max(h, -std::max(0.0, offset)), limit_, 0.0);
        content_ = std::max(0.0, offset + height_offset_);
        content_offset_ -= content_ - before;
        if (content_ <= 0.0) {
            content_offset_ = 0.0;
        }
    }

    bool needs_settle() const {
        if (behavior_ == TopAppBarScroll::pinned || settling_) {
            return false;
        }
        const double f = collapsed_fraction();
        return f >= current_style().top_app_bar.common.settle_threshold && f < 1.0 && height_offset_ > limit_ && height_offset_ < 0.0;
    }
    void settle() {
        if (!needs_settle()) {
            return;
        }
        settling_ = true;
        settle_.snap_to(height_offset_);
        settle_.animate_to(collapsed_fraction() < current_style().top_app_bar.common.settle_midpoint ? 0.0 : limit_);
    }
    void cancel_settle() {
        settling_ = false;
        last_offset_ = scroll_offset();
    }
    bool settling() const { return settling_; }
    void step(double dt_seconds) {
        if (!settling_) {
            return;
        }
        settle_.step(dt_seconds);
        height_offset_ = std::clamp(settle_.value(), limit_, 0.0);
        if (settle_.settled()) {
            height_offset_ = settle_.target();
            settling_ = false;
        }
        last_offset_ = scroll_offset();
    }

private:
    TopAppBarScroll behavior_;
    double limit_ = 0.0;
    double height_offset_ = 0.0;
    double content_offset_ = 0.0;
    double content_ = 0.0;
    double last_offset_ = 0.0;
    bool settling_ = false;
    motion::Animated settle_;
};

}
