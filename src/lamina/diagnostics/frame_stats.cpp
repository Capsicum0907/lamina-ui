#include "lamina/diagnostics/frame_stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lamina::diagnostics {

namespace {

constexpr std::array<std::string_view, stage_count> stage_names{"wait",          "events",      "update", "layout",  "paint",  "image copy",
                                                                "raster update", "raster draw", "upload", "present", "overlay"};
constexpr std::array<std::string_view, count_field_count> count_names{"paths",     "offscreen", "points",    "shadows", "clips",   "images",
                                                                      "image MB",  "image Mpx", "loads",   "load MB", "cover Mpx"};

std::string format(const char* pattern, double value) {
    std::array<char, 32> out{};
    std::snprintf(out.data(), out.size(), pattern, value);
    return out.data();
}

void add(Spread& s, double value) {
    s.average += value;
    s.worst = std::max(s.worst, value);
}

void finish(Spread& s, std::size_t n) {
    if (n > 0) {
        s.average /= static_cast<double>(n);
    }
}

double refresh_or_fallback(double refresh_hz, const tokens::FrameStatsTokens& t) {
    return refresh_hz > 0.0 ? refresh_hz : t.fallback_refresh_hz;
}

}

std::string_view stage_name(Stage stage) {
    return stage_names[static_cast<std::size_t>(stage)];
}

bool is_work_stage(Stage stage) {
    return stage != Stage::wait && stage != Stage::present && stage != Stage::count;
}

std::string_view count_name(CountField field) {
    return count_names[static_cast<std::size_t>(field)];
}

double count_value(const FrameCounts& c, CountField field) {
    switch (field) {
    case CountField::paths: return static_cast<double>(c.paths);
    case CountField::offscreen_paths: return static_cast<double>(c.offscreen_paths);
    case CountField::path_points: return static_cast<double>(c.path_points);
    case CountField::shadows: return static_cast<double>(c.shadows);
    case CountField::clips: return static_cast<double>(c.clips);
    case CountField::images: return static_cast<double>(c.images);
    case CountField::image_bytes: return static_cast<double>(c.image_bytes);
    case CountField::image_pixels: return static_cast<double>(c.image_pixels);
    case CountField::loads: return static_cast<double>(c.loads);
    case CountField::load_bytes: return static_cast<double>(c.load_bytes);
    case CountField::covered_pixels: return c.covered_pixels;
    case CountField::count: break;
    }
    return 0.0;
}

double work_seconds(const FrameSample& f) {
    double work = 0.0;
    for (std::size_t i = 0; i < stage_count; ++i) {
        if (is_work_stage(static_cast<Stage>(i))) {
            work += f.seconds[i];
        }
    }
    return work;
}

Stage top_stage(const FrameSample& f) {
    std::size_t best = static_cast<std::size_t>(Stage::events);
    for (std::size_t i = 0; i < stage_count; ++i) {
        if (is_work_stage(static_cast<Stage>(i)) && f.seconds[i] > f.seconds[best]) {
            best = i;
        }
    }
    return static_cast<Stage>(best);
}

std::size_t missed_refreshes(double interval_seconds, double refresh_hz) {
    if (refresh_hz <= 0.0 || interval_seconds <= 0.0) {
        return 0;
    }
    const double periods = std::round(interval_seconds * refresh_hz);
    return periods > 1.0 ? static_cast<std::size_t>(periods) - 1 : 0;
}

FrameSummary summarize(std::span<const FrameSample> samples, double refresh_hz, const tokens::FrameStatsTokens& t) {
    FrameSummary out;
    out.frames = samples.size();
    std::vector<double> intervals;
    const double hz = refresh_or_fallback(refresh_hz, t);
    double worst_key = -1.0;
    for (std::size_t n = 0; n < samples.size(); ++n) {
        const FrameSample& f = samples[n];
        for (std::size_t i = 0; i < stage_count; ++i) {
            add(out.stages[i], f.seconds[i]);
        }
        const double work = work_seconds(f);
        add(out.work, work);
        const double key = f.interval_seconds ? *f.interval_seconds : work;
        if (key > worst_key) {
            worst_key = key;
            out.worst_frame = f;
        }
        for (std::size_t i = 0; i < count_field_count; ++i) {
            add(out.counts[i], count_value(f.counts, static_cast<CountField>(i)));
        }
        if (f.interval_seconds) {
            const double v = *f.interval_seconds;
            intervals.push_back(v);
            add(out.interval, v);
            if (v * hz > t.late_factor) {
                out.late += 1;
                if (out.late_frames.size() < t.listed_late_frames) {
                    const Stage top = top_stage(f);
                    out.late_frames.push_back({n, v, work, top, f.seconds[static_cast<std::size_t>(top)], f.counts.loads});
                }
            }
            out.missed_refreshes += missed_refreshes(v, hz);
        }
    }
    for (Spread& s : out.stages) {
        finish(s, out.frames);
    }
    finish(out.work, out.frames);
    for (Spread& s : out.counts) {
        finish(s, out.frames);
    }
    out.intervals = intervals.size();
    finish(out.interval, out.intervals);
    if (!intervals.empty()) {
        const auto mid = intervals.begin() + static_cast<std::ptrdiff_t>(intervals.size() / 2);
        std::nth_element(intervals.begin(), mid, intervals.end());
        out.median_interval = *mid;
    }
    return out;
}

std::optional<FrameStatsSwitches> parse_frame_stats(std::string_view value) {
    if (value == frame_stats_on) {
        return FrameStatsSwitches{true, true};
    }
    if (value == frame_stats_log_only) {
        return FrameStatsSwitches{false, true};
    }
    if (value == frame_stats_off) {
        return FrameStatsSwitches{};
    }
    return std::nullopt;
}

std::vector<ReportRow> report_rows(const FrameSummary& s, const DisplayInfo& d, std::string_view title, const tokens::FrameStatsTokens& t) {
    const double ms = t.seconds_to_ms;
    auto ms_of = [&](double seconds) { return format("%.2f", seconds * ms); };
    std::vector<ReportRow> rows;
    rows.push_back({std::string(title) + " " + std::to_string(s.frames) + " frames, " + std::to_string(s.intervals) + " intervals"});
    rows.push_back({"interval ms avg " + format("%.2f", s.interval.average * ms) + " med " + format("%.2f", s.median_interval * ms) + " worst " +
                    format("%.2f", s.interval.worst * ms)});
    rows.push_back({"late " + std::to_string(s.late) + " missed " + std::to_string(s.missed_refreshes) + " at " + format("%.1f", d.refresh_hz) +
                    " Hz, budget " + format("%.2f", d.refresh_hz > 0.0 ? ms / d.refresh_hz : 0.0) + " ms"});
    rows.push_back({d.renderer + " vsync " + (d.vsync ? std::to_string(*d.vsync) : std::string("?")) + ", window " + std::to_string(d.window_width) +
                    "x" + std::to_string(d.window_height) + ", canvas " + std::to_string(d.canvas_width) + "x" +
                    std::to_string(d.canvas_height) + " " +
                    format("%.2f", static_cast<double>(d.canvas_width) * d.canvas_height / t.pixels_per_megapixel) + " Mpx"});
    rows.push_back({"stage ms", "avg", "worst", "per frame", "avg", "worst"});
    auto count_text = [&](CountField f, double v) {
        if (f == CountField::image_bytes || f == CountField::load_bytes) {
            return format("%.1f", v / t.bytes_per_megabyte);
        }
        if (f == CountField::image_pixels || f == CountField::covered_pixels) {
            return format("%.2f", v / t.pixels_per_megapixel);
        }
        return format("%.0f", v);
    };
    const std::size_t lines = std::max(stage_count + 1, count_field_count);
    for (std::size_t i = 0; i < lines; ++i) {
        ReportRow row;
        if (i < stage_count) {
            row = {std::string(stage_name(static_cast<Stage>(i))), ms_of(s.stages[i].average), ms_of(s.stages[i].worst)};
        } else if (i == stage_count) {
            row = {"work", ms_of(s.work.average), ms_of(s.work.worst)};
        } else {
            row = {"", "", ""};
        }
        if (i < count_field_count) {
            const auto f = static_cast<CountField>(i);
            row.push_back(std::string(count_name(f)));
            row.push_back(count_text(f, s.counts[i].average));
            row.push_back(count_text(f, s.counts[i].worst));
        } else {
            row.insert(row.end(), {"", "", ""});
        }
        rows.push_back(std::move(row));
    }
    auto chunked = [&](std::string head, const std::vector<std::string>& items) {
        std::string line = std::move(head);
        std::size_t in_line = 0;
        for (const std::string& item : items) {
            if (in_line == t.report_items_per_row) {
                rows.push_back({line});
                line.clear();
                in_line = 0;
            }
            line += (line.empty() ? "" : " ") + item;
            in_line += 1;
        }
        if (!line.empty()) {
            rows.push_back({line});
        }
    };
    if (s.worst_frame) {
        const FrameSample& w = *s.worst_frame;
        std::string head = "worst frame:";
        if (w.interval_seconds) {
            head += " interval " + ms_of(*w.interval_seconds);
        }
        head += " work " + ms_of(work_seconds(w)) + " paths " + std::to_string(w.counts.paths) + " images " + std::to_string(w.counts.images) +
                " loads " + std::to_string(w.counts.loads);
        rows.push_back({head});
        std::vector<std::string> items;
        for (std::size_t i = 0; i < stage_count; ++i) {
            const auto st = static_cast<Stage>(i);
            if (w.seconds[i] > 0.0 && st != Stage::wait) {
                items.push_back(std::string(stage_name(st)) + " " + ms_of(w.seconds[i]));
            }
        }
        chunked("", items);
    }
    if (!s.late_frames.empty()) {
        std::vector<std::string> items;
        for (const LateFrame& l : s.late_frames) {
            items.push_back("#" + std::to_string(l.index) + " " + format("%.1f", l.interval_seconds * ms) + "/" + format("%.1f", l.work_seconds * ms) + " " +
                            std::string(stage_name(l.top)) + (l.loads > 0 ? " L" + std::to_string(l.loads) : ""));
        }
        rows.push_back({"late frames: #frame interval/work ms, top stage, L loads"});
        chunked("", items);
    }
    return rows;
}

std::string report_line(const std::vector<ReportRow>& rows) {
    std::string out;
    for (const ReportRow& row : rows) {
        std::string line;
        for (const std::string& cell : row) {
            if (cell.empty()) {
                continue;
            }
            line += (line.empty() ? "" : " ") + cell;
        }
        if (line.empty()) {
            continue;
        }
        out += (out.empty() ? "" : " | ") + line;
    }
    return out;
}

FrameRing::FrameRing(std::size_t capacity) : samples_(std::max<std::size_t>(capacity, 1)) {}

void FrameRing::push(const FrameSample& sample) {
    samples_[next_] = sample;
    next_ = (next_ + 1) % samples_.size();
    size_ = std::min(size_ + 1, samples_.size());
}

void FrameRing::clear() {
    next_ = 0;
    size_ = 0;
}

std::vector<FrameSample> FrameRing::ordered() const {
    std::vector<FrameSample> out;
    out.reserve(size_);
    const std::size_t start = (next_ + samples_.size() - size_) % samples_.size();
    for (std::size_t i = 0; i < size_; ++i) {
        out.push_back(samples_[(start + i) % samples_.size()]);
    }
    return out;
}

FrameProbe::FrameProbe(const tokens::FrameStatsTokens& tokens) : tokens_(&tokens), ring_(tokens.recent_frames) {}

void FrameProbe::set_switches(FrameStatsSwitches switches) {
    const bool was_active = active();
    switches_ = switches;
    if (!switches_.log) {
        pending_log_.clear();
    }
    if (active() != was_active) {
        ring_.clear();
        pending_log_.clear();
        last_end_.reset();
        in_frame_ = false;
    }
}

double FrameProbe::idle_wait_seconds() const {
    return tokens_->idle_wait_refreshes / refresh_or_fallback(display_.refresh_hz, *tokens_);
}

void FrameProbe::begin_frame(double waited_seconds) {
    if (!active()) {
        return;
    }
    continuous_ = last_end_.has_value() && waited_seconds <= idle_wait_seconds();
    current_ = {};
    current_.seconds[static_cast<std::size_t>(Stage::wait)] = continuous_ ? waited_seconds : 0.0;
    in_frame_ = true;
    stage_ = Stage::events;
    stage_start_ = Clock::now();
}

Stage FrameProbe::enter(Stage stage) {
    if (!in_frame_) {
        return stage;
    }
    const Clock::time_point now = Clock::now();
    current_.seconds[static_cast<std::size_t>(stage_)] += std::chrono::duration<double>(now - stage_start_).count();
    const Stage previous = stage_;
    stage_ = stage;
    stage_start_ = now;
    return previous;
}

void FrameProbe::end_frame() {
    if (!in_frame_) {
        return;
    }
    const Clock::time_point now = Clock::now();
    current_.seconds[static_cast<std::size_t>(stage_)] += std::chrono::duration<double>(now - stage_start_).count();
    if (continuous_ && last_end_) {
        current_.interval_seconds = std::chrono::duration<double>(now - *last_end_).count();
    }
    last_end_ = now;
    ring_.push(current_);
    if (switches_.log) {
        pending_log_.push_back(current_);
    }
    in_frame_ = false;
}

FrameSummary FrameProbe::recent() const {
    const std::vector<FrameSample> samples = ring_.ordered();
    return summarize(samples, display_.refresh_hz, *tokens_);
}

std::optional<FrameSummary> FrameProbe::take_log(bool going_idle) {
    if (!switches_.log) {
        return std::nullopt;
    }
    const bool full = pending_log_.size() >= tokens_->log_frames;
    const bool burst = going_idle && pending_log_.size() >= tokens_->log_min_frames;
    if (!full && !burst) {
        if (going_idle) {
            pending_log_.clear();
        }
        return std::nullopt;
    }
    FrameSummary out = summarize(pending_log_, display_.refresh_hz, *tokens_);
    pending_log_.clear();
    return out;
}

StageScope::StageScope(FrameProbe* probe, Stage stage) : probe_(probe && probe->in_frame() ? probe : nullptr) {
    if (probe_) {
        previous_ = probe_->enter(stage);
    }
}

StageScope::~StageScope() {
    if (probe_) {
        probe_->enter(previous_);
    }
}

}
