#pragma once

#include <cstddef>
#include <string_view>

#include "lamina/color/scheme.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::tokens {

enum class ScreenCorner { top_start, top_end, bottom_start, bottom_end };

struct FrameStatsTokens {
    std::size_t recent_frames;
    std::size_t log_frames;
    std::size_t log_min_frames;
    std::size_t listed_late_frames;
    std::size_t report_items_per_row;
    double late_factor;
    double idle_wait_refreshes;
    double fallback_refresh_hz;
    std::string_view log_tag;
    double seconds_to_ms;
    double bytes_per_megabyte;
    double pixels_per_megapixel;
    ScreenCorner corner;
    double margin;
    double padding;
    double column_gap;
    double corner_radius;
    double history_height;
    double history_bar_width;
    double history_refreshes;
    double history_rule_width;
    color::Role history_bar;
    color::Role history_late;
    TypeRole type;
    color::Role container;
    double container_opacity;
    color::Role content;
};

inline constexpr FrameStatsTokens frame_stats{
    .recent_frames = 120,
    .log_frames = 240,
    .log_min_frames = 5,
    .listed_late_frames = 8,
    .report_items_per_row = 2,
    .late_factor = 1.5,
    .idle_wait_refreshes = 2.0,
    .fallback_refresh_hz = 60.0,
    .log_tag = "lamina.frames",
    .seconds_to_ms = 1000.0,
    .bytes_per_megabyte = 1024.0 * 1024.0,
    .pixels_per_megapixel = 1e6,
    .corner = ScreenCorner::bottom_start,
    .margin = 8.0,
    .padding = 8.0,
    .column_gap = 10.0,
    .corner_radius = 8.0,
    .history_height = 36.0,
    .history_bar_width = 2.0,
    .history_refreshes = 4.0,
    .history_rule_width = 1.0,
    .history_bar = color::Role::inverse_on_surface,
    .history_late = color::Role::error,
    .type = TypeRole::label_small,
    .container = color::Role::inverse_surface,
    .container_opacity = 0.88,
    .content = color::Role::inverse_on_surface,
};

}
