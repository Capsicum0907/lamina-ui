#pragma once

#include <cstddef>

#include "lamina/motion/paging.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/tokens/drag.hpp"

namespace lamina::tokens {

struct PagerTokens {
    double touch_slop;
    double horizontal_angle_limit_degrees;
    motion::PageSnapSpec snap;
    double catch_distance;
    std::size_t beyond_viewport_pages;
    std::size_t animate_jump_pages;
    double page_spacing;
    double focus_ring_corner;
    motion::VelocityTrackerSpec tracking;
    double maximum_velocity;
    motion::SpringSpec settle_spring;
    motion::SettleThreshold settle_threshold;
};

inline constexpr PagerTokens pager{
    .touch_slop = touch_slop * 2.0,
    .horizontal_angle_limit_degrees = touch_scroll.horizontal_angle_limit_degrees,
    .snap = {.min_fling_velocity = 400.0, .min_fling_distance = 25.0, .positional_threshold = 0.6},
    .catch_distance = 2.0,
    .beyond_viewport_pages = 1,
    .animate_jump_pages = 1,
    .page_spacing = 0.0,
    .focus_ring_corner = 0.0,
    .tracking = velocity_tracking,
    .maximum_velocity = maximum_fling_velocity,
    .settle_spring = {1.0, 400.0},
    .settle_threshold = {1e-3, 1e-2},
};

}
