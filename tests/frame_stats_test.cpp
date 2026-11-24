#include <doctest/doctest.h>

#include <vector>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;
using namespace lamina::diagnostics;

namespace {

constexpr double hz = 120.0;
constexpr double period = 1.0 / hz;

FrameSample sample(double interval, double paint) {
    FrameSample s;
    s.interval_seconds = interval;
    s.seconds[static_cast<std::size_t>(Stage::paint)] = paint;
    s.seconds[static_cast<std::size_t>(Stage::present)] = period;
    s.seconds[static_cast<std::size_t>(Stage::wait)] = period;
    return s;
}

}

TEST_CASE("missed refreshes round the interval to whole refresh periods") {
    CHECK(missed_refreshes(period, hz) == 0);
    CHECK(missed_refreshes(period * 1.4, hz) == 0);
    CHECK(missed_refreshes(period * 2.0, hz) == 1);
    CHECK(missed_refreshes(period * 3.2, hz) == 2);
    CHECK(missed_refreshes(period * 3.0, 0.0) == 0);
}

TEST_CASE("a summary counts late frames, the median interval and work without wait or present") {
    const std::vector<FrameSample> samples{sample(period, 0.002), sample(period * 2.0, 0.010), sample(period, 0.003)};
    const FrameSummary s = summarize(samples, hz);
    CHECK(s.frames == 3);
    CHECK(s.intervals == 3);
    CHECK(s.late == 1);
    CHECK(s.missed_refreshes == 1);
    CHECK(s.median_interval == doctest::Approx(period));
    CHECK(s.interval.worst == doctest::Approx(period * 2.0));
    CHECK(s.work.worst == doctest::Approx(0.010));
    CHECK(s.work.average == doctest::Approx(0.005));
    CHECK(s.stages[static_cast<std::size_t>(Stage::paint)].worst == doctest::Approx(0.010));
}

TEST_CASE("a frame without an interval adds to the stages but not to the intervals") {
    FrameSample idle = sample(0.0, 0.004);
    idle.interval_seconds.reset();
    const std::vector<FrameSample> samples{idle, sample(period, 0.002)};
    const FrameSummary s = summarize(samples, hz);
    CHECK(s.frames == 2);
    CHECK(s.intervals == 1);
    CHECK(s.late == 0);
    CHECK(s.stages[static_cast<std::size_t>(Stage::paint)].worst == doctest::Approx(0.004));
}

TEST_CASE("the ring keeps the latest frames in order") {
    FrameRing ring(3);
    for (int i = 1; i <= 5; ++i) {
        ring.push(sample(period * i, 0.0));
    }
    const std::vector<FrameSample> kept = ring.ordered();
    REQUIRE(kept.size() == 3);
    CHECK(*kept[0].interval_seconds == doctest::Approx(period * 3));
    CHECK(*kept[2].interval_seconds == doctest::Approx(period * 5));
}

TEST_CASE("the probe does nothing until it is switched on") {
    FrameProbe probe;
    probe.begin_frame(0.0);
    CHECK_FALSE(probe.in_frame());
    CHECK_FALSE(probe.counting());
    probe.end_frame();
    CHECK(probe.recent().frames == 0);
}

TEST_CASE("the probe measures an interval only between continuous frames") {
    FrameProbe probe;
    probe.set_switches({true, false});
    probe.set_display({.refresh_hz = hz});
    probe.begin_frame(0.0);
    {
        const StageScope stage(&probe, Stage::paint);
        CHECK(probe.counting());
    }
    probe.end_frame();
    probe.begin_frame(0.0);
    probe.end_frame();
    probe.begin_frame(1.0);
    probe.end_frame();
    const FrameSummary s = probe.recent();
    CHECK(s.frames == 3);
    CHECK(s.intervals == 1);
}

TEST_CASE("a report frame is neither recorded nor counted") {
    FrameProbe probe;
    probe.set_switches({true, false});
    probe.begin_frame(0.0, false);
    CHECK(probe.in_frame());
    CHECK(probe.report_frame());
    CHECK_FALSE(probe.counting());
    probe.end_frame();
    CHECK(probe.recent().frames == 0);
}

TEST_CASE("a gesture ends with a log line and a report frame, and a long run logs every log_frames") {
    tokens::FrameStatsTokens t = tokens::frame_stats;
    t.log_frames = 4;
    t.log_min_frames = 2;
    FrameProbe probe(t);
    probe.set_switches({true, true});
    auto frames = [&](int n) {
        for (int i = 0; i < n; ++i) {
            probe.begin_frame(0.0);
            probe.end_frame();
        }
    };
    CHECK_FALSE(probe.end_gesture().report);
    frames(1);
    const auto short_one = probe.end_gesture();
    CHECK(short_one.report);
    CHECK_FALSE(short_one.log);
    CHECK_FALSE(probe.last_gesture());
    probe.begin_frame(t.run_gap_seconds * 2.0);
    probe.end_frame();
    frames(2);
    CHECK_FALSE(probe.take_running());
    frames(1);
    const auto running = probe.take_running();
    REQUIRE(running);
    CHECK(running->frames == 4);
    frames(2);
    const auto end = probe.end_gesture();
    REQUIRE(end.log);
    CHECK(end.log->frames == 2);
    REQUIRE(probe.last_gesture());
    CHECK(probe.last_gesture()->frames == 6);
    CHECK_FALSE(probe.end_gesture().report);
}

TEST_CASE("without the overlay a gesture end asks for no report frame") {
    FrameProbe probe;
    probe.set_switches({false, true});
    probe.begin_frame(0.0);
    probe.end_frame();
    CHECK_FALSE(probe.end_gesture().report);
}

TEST_CASE("the canvas counts paths, images and the bytes the ThorVG canvas copies") {
    thorvg::Canvas canvas(100, 100);
    FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    probe.begin_frame(0.0);
    canvas.clear(0xffffffff);
    canvas.fill_rect({0.0, 0.0, 50.0, 20.0}, 0xff000000);
    canvas.fill_rounded_rect({0.0, 0.0, 200.0, 200.0}, render::uniform_radii(4.0), 0xff000000);
    render::Image image{4, 2, std::vector<color::Argb>(8, 0xffff0000)};
    canvas.draw_image(image, {10.0, 10.0, 40.0, 20.0});
    canvas.push_clip(render::Rect{0.0, 0.0, 10.0, 10.0});
    canvas.pop_clip();
    const FrameCounts c = probe.counts();
    CHECK(c.paths == 2);
    CHECK(c.images == 1);
    CHECK(c.image_pixels == 8);
    CHECK(c.image_bytes == 32);
    CHECK(c.clips == 1);
    CHECK(c.covered_pixels == doctest::Approx(50.0 * 20.0 + 100.0 * 100.0 + 40.0 * 20.0));
    canvas.finish();
    probe.end_frame();
    CHECK(probe.recent().stages[static_cast<std::size_t>(Stage::raster_draw)].worst > 0.0);
}

TEST_CASE("frame stats values parse") {
    CHECK(parse_frame_stats("on")->hud);
    CHECK(parse_frame_stats("on")->log);
    CHECK_FALSE(parse_frame_stats("log")->hud);
    CHECK(parse_frame_stats("log")->log);
    CHECK_FALSE(parse_frame_stats("off")->log);
    CHECK_FALSE(parse_frame_stats("yes"));
}

TEST_CASE("the report has one line per stage and the log line joins the rows") {
    const std::vector<FrameSample> samples{sample(period, 0.002)};
    const auto rows = report_rows(summarize(samples, hz), {.renderer = "software", .vsync = 1, .refresh_hz = hz}, "test");
    std::size_t stage_rows = 0;
    for (const ReportRow& row : rows) {
        if (row.size() > 1 && !row.front().empty()) {
            stage_rows += 1;
        }
    }
    CHECK(stage_rows == stage_count + 2);
    const std::string line = report_line(rows);
    CHECK(line.find("paint 2.00") != std::string::npos);
    CHECK(line.find("software vsync 1") != std::string::npos);
}

TEST_CASE("frames the program paces with short wakes, such as a playing video, count as one run with their intervals") {
    FrameProbe probe;
    probe.set_switches({true, false});
    probe.set_display({.refresh_hz = hz});
    const double frame_wait = 0.04;
    CHECK(probe.paced_wake(frame_wait));
    CHECK_FALSE(probe.paced_wake(0.5));
    CHECK_FALSE(probe.paced_wake(-1.0));
    probe.begin_frame(0.0);
    probe.end_frame();
    for (int i = 0; i < 4; ++i) {
        probe.begin_frame(frame_wait, true, true);
        probe.end_frame();
    }
    CHECK(probe.recent().intervals == 4);
    probe.begin_frame(frame_wait, true, false);
    probe.end_frame();
    CHECK(probe.recent().intervals == 4);
}
