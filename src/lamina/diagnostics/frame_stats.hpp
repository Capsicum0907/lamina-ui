#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/tokens/frame_stats.hpp"

namespace lamina::diagnostics {

enum class Stage : std::size_t { wait, events, update, layout, paint, image_copy, raster_update, raster_draw, upload, present, overlay, count };

inline constexpr std::size_t stage_count = static_cast<std::size_t>(Stage::count);

std::string_view stage_name(Stage stage);
bool is_work_stage(Stage stage);

struct FrameCounts {
    std::uint64_t paths = 0;
    std::uint64_t offscreen_paths = 0;
    std::uint64_t path_points = 0;
    std::uint64_t shadows = 0;
    std::uint64_t clips = 0;
    std::uint64_t images = 0;
    std::uint64_t image_bytes = 0;
    std::uint64_t image_pixels = 0;
    std::uint64_t loads = 0;
    std::uint64_t load_bytes = 0;
    double covered_pixels = 0.0;
};

struct FrameSample {
    std::array<double, stage_count> seconds{};
    std::optional<double> interval_seconds;
    FrameCounts counts;
};

struct DisplayInfo {
    std::string renderer;
    std::optional<int> vsync;
    double refresh_hz = 0.0;
    int window_width = 0;
    int window_height = 0;
    int canvas_width = 0;
    int canvas_height = 0;
};

enum class CountField : std::size_t { paths, offscreen_paths, path_points, shadows, clips, images, image_bytes, image_pixels, loads, load_bytes, covered_pixels, count };

inline constexpr std::size_t count_field_count = static_cast<std::size_t>(CountField::count);

std::string_view count_name(CountField field);
double count_value(const FrameCounts& counts, CountField field);

struct Spread {
    double average = 0.0;
    double worst = 0.0;
};

struct LateFrame {
    std::size_t index = 0;
    double interval_seconds = 0.0;
    double work_seconds = 0.0;
    Stage top = Stage::paint;
    double top_seconds = 0.0;
    std::uint64_t loads = 0;
};

double work_seconds(const FrameSample& sample);
Stage top_stage(const FrameSample& sample);

struct FrameSummary {
    std::size_t frames = 0;
    std::array<Spread, stage_count> stages{};
    Spread work;
    std::size_t intervals = 0;
    Spread interval;
    double median_interval = 0.0;
    std::size_t late = 0;
    std::size_t missed_refreshes = 0;
    std::array<Spread, count_field_count> counts{};
    std::optional<FrameSample> worst_frame;
    std::vector<LateFrame> late_frames;
};

FrameSummary summarize(std::span<const FrameSample> samples, double refresh_hz, const tokens::FrameStatsTokens& tokens = tokens::frame_stats);

std::size_t missed_refreshes(double interval_seconds, double refresh_hz);

struct FrameStatsSwitches {
    bool hud = false;
    bool log = false;
};

inline constexpr std::string_view frame_stats_on = "on";
inline constexpr std::string_view frame_stats_log_only = "log";
inline constexpr std::string_view frame_stats_off = "off";

std::optional<FrameStatsSwitches> parse_frame_stats(std::string_view value);

using ReportRow = std::vector<std::string>;

std::vector<ReportRow> report_rows(const FrameSummary& summary, const DisplayInfo& display, std::string_view title,
                                   const tokens::FrameStatsTokens& tokens = tokens::frame_stats);
std::string report_line(const std::vector<ReportRow>& rows);

class FrameRing {
public:
    explicit FrameRing(std::size_t capacity);
    void push(const FrameSample& sample);
    void clear();
    std::vector<FrameSample> ordered() const;
    std::size_t size() const { return size_; }

private:
    std::vector<FrameSample> samples_;
    std::size_t next_ = 0;
    std::size_t size_ = 0;
};

class FrameProbe {
public:
    explicit FrameProbe(const tokens::FrameStatsTokens& tokens = tokens::frame_stats);

    bool active() const { return switches_.hud || switches_.log; }
    const FrameStatsSwitches& switches() const { return switches_; }
    void set_switches(FrameStatsSwitches switches);
    const tokens::FrameStatsTokens& tokens() const { return *tokens_; }

    void begin_frame(double waited_seconds);
    double idle_wait_seconds() const;
    Stage enter(Stage stage);
    void end_frame();
    bool in_frame() const { return in_frame_; }
    bool counting() const { return active() && in_frame_ && stage_ != Stage::overlay; }
    FrameCounts& counts() { return current_.counts; }

    void set_display(DisplayInfo display) { display_ = std::move(display); }
    const DisplayInfo& display() const { return display_; }

    FrameSummary recent() const;
    std::vector<FrameSample> history() const { return ring_.ordered(); }
    std::optional<FrameSummary> take_log(bool going_idle);

private:
    using Clock = std::chrono::steady_clock;

    const tokens::FrameStatsTokens* tokens_;
    FrameStatsSwitches switches_;
    DisplayInfo display_;
    FrameRing ring_;
    std::vector<FrameSample> pending_log_;
    FrameSample current_;
    bool in_frame_ = false;
    bool continuous_ = false;
    Stage stage_ = Stage::events;
    Clock::time_point stage_start_{};
    std::optional<Clock::time_point> last_end_;
};

class StageScope {
public:
    StageScope(FrameProbe* probe, Stage stage);
    ~StageScope();
    StageScope(const StageScope&) = delete;
    StageScope& operator=(const StageScope&) = delete;

private:
    FrameProbe* probe_;
    Stage previous_ = Stage::events;
};

}
