#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

#include "lamina/widgets/protocol.hpp"
#include "lamina/widgets/wavy_progress.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::CircularWavyProgressIndicator;
using widgets::LinearWavyProgressIndicator;

namespace {

template <typename W>
void run(W& w, double seconds) {
    const int n = static_cast<int>(std::lround(seconds / frame_seconds));
    for (int i = 0; i < n; ++i) {
        w.step(frame_seconds);
    }
}

double max_deviation(const std::vector<std::vector<render::Point>>& lines, double centre) {
    double d = 0.0;
    for (const auto& line : lines) {
        for (const render::Point& p : line) {
            d = std::max(d, std::abs(p.y - centre));
        }
    }
    return d;
}

}

static_assert(widgets::Sized<LinearWavyProgressIndicator>);
static_assert(widgets::WidthSettable<LinearWavyProgressIndicator>);
static_assert(widgets::Sized<CircularWavyProgressIndicator>);

TEST_CASE("wavy progress tokens follow {Linear,Circular}ProgressIndicatorTokens.kt and WavyProgressIndicator.kt") {
    const auto& t = tokens::wavy_progress_tokens;
    CHECK(t.indicator == color::Role::primary);
    CHECK(t.track == color::Role::secondary_container);
    CHECK(t.linear_height == 10.0);
    CHECK(t.linear_wavelength == 40.0);
    CHECK(t.linear_indeterminate_wavelength == 20.0);
    CHECK(t.linear_stop_size == 4.0);
    CHECK(t.circular_size == 48.0);
    CHECK(t.circular_wavelength == 15.0);
    CHECK(tokens::wavy_progress_motion.amplitude_up.duration_seconds == doctest::Approx(0.5));
    CHECK(tokens::linear_indeterminate.cycle_ms == 1750.0);
    CHECK(tokens::circular_indeterminate.cycle_ms == 6000.0);
}

TEST_CASE("the default amplitude is flat below 10 % and from 95 %") {
    CHECK(widgets::wavy_indicator_amplitude(0.1) == 0.0);
    CHECK(widgets::wavy_indicator_amplitude(0.11) == 1.0);
    CHECK(widgets::wavy_indicator_amplitude(0.94) == 1.0);
    CHECK(widgets::wavy_indicator_amplitude(0.95) == 0.0);
}

TEST_CASE("the indeterminate keyframes follow ProgressIndicator.kt") {
    const auto& l = tokens::linear_indeterminate;
    CHECK(widgets::linear_indeterminate_fraction(l.first_head, 0.0) == 0.0);
    CHECK(widgets::linear_indeterminate_fraction(l.first_head, 1000.0) == doctest::Approx(1.0));
    CHECK(widgets::linear_indeterminate_fraction(l.first_tail, 250.0) == 0.0);
    CHECK(widgets::linear_indeterminate_fraction(l.second_head, 600.0) == 0.0);
    const double mid = widgets::linear_indeterminate_fraction(l.first_head, 500.0);
    CHECK(mid > 0.0);
    CHECK(mid < 0.5);
    CHECK(widgets::circular_indeterminate_sweep(0.0) == doctest::Approx(0.1));
    CHECK(widgets::circular_indeterminate_sweep(3000.0) == doctest::Approx(0.87));
    CHECK(widgets::circular_indeterminate_sweep(5999.0) == doctest::Approx(0.1).epsilon(0.01));
    CHECK(widgets::circular_indeterminate_rotation(0.0) == doctest::Approx(90.0));
    CHECK(widgets::circular_indeterminate_rotation(300.0) == doctest::Approx(54.0 + 90.0 + 90.0));
    CHECK(widgets::circular_indeterminate_rotation(1500.0) == doctest::Approx(270.0 + 90.0 + 90.0));
}

TEST_CASE("the amplitude ramps on 500 ms tweens, standard up and emphasized-accelerate down") {
    LinearWavyProgressIndicator p({.progress = 0.05});
    CHECK(p.amplitude() == 0.0);
    p.set_progress(0.5);
    run(p, 0.25);
    CHECK(p.amplitude() > 0.5);
    CHECK(p.amplitude() < 1.0);
    run(p, 0.3);
    CHECK(p.amplitude() == doctest::Approx(1.0));
    p.set_progress(0.97);
    run(p, 0.25);
    CHECK(p.amplitude() > 0.6);
    run(p, 0.3);
    CHECK(p.amplitude() == doctest::Approx(0.0));
}

TEST_CASE("a determinate linear indicator draws its wave up to the progress, a 4 dp gap, the track and the stop dot") {
    LinearWavyProgressIndicator p({.progress = 0.5});
    p.set_origin({0.0, 0.0});
    run(p, 0.6);
    const auto active = p.active_lines();
    REQUIRE(active.size() == 1);
    CHECK(std::abs(active[0].front().x - 2.0) < 0.5);
    CHECK(active[0].back().x == doctest::Approx(120.0).epsilon(0.03));
    CHECK(max_deviation(active, 5.0) == doctest::Approx(3.0).epsilon(0.1));
    const auto track = p.track_lines();
    REQUIRE(track.size() == 1);
    CHECK(track[0].front().x == doctest::Approx(238.0));
    CHECK(track[0].back().x == doctest::Approx(120.0 + 4.0 + 4.0));
    const auto dot = p.stop_indicator();
    REQUIRE(dot);
    CHECK(dot->width == 4.0);
    CHECK(dot->x == doctest::Approx(236.0));
}

TEST_CASE("the stop dot shrinks once the progress reaches it") {
    LinearWavyProgressIndicator p({.progress = 0.99});
    const auto dot = p.stop_indicator();
    if (dot) {
        CHECK(dot->width < 4.0);
    }
    p.set_progress(1.0);
    CHECK_FALSE(p.stop_indicator());
}

TEST_CASE("a flat amplitude draws a straight line") {
    LinearWavyProgressIndicator p({.progress = 0.05});
    CHECK(max_deviation(p.active_lines(), 5.0) == doctest::Approx(0.0));
}

TEST_CASE("the wave travels one wavelength a second") {
    LinearWavyProgressIndicator p({.progress = 0.5});
    const double before = p.wave_offset();
    run(p, 0.25);
    CHECK(p.wave_offset() - before == doctest::Approx(0.25).epsilon(0.05));
}

TEST_CASE("an indeterminate linear indicator draws two lines with the 20 dp wavelength") {
    LinearWavyProgressIndicator p;
    CHECK_FALSE(p.determinate());
    CHECK(p.wavelength() == 20.0);
    CHECK(p.animating());
    run(p, 1.2);
    CHECK(p.active_lines().size() == 2);
    CHECK_FALSE(p.stop_indicator());
    CHECK_FALSE(p.accessible().state.range);
}

TEST_CASE("switching a linear indicator to determinate grows the bar in over 500 ms") {
    LinearWavyProgressIndicator p;
    run(p, 0.5);
    p.set_progress(0.6);
    CHECK(p.fractions()[1] == doctest::Approx(0.0));
    run(p, 0.25);
    CHECK(p.fractions()[1] == doctest::Approx(0.3).epsilon(0.05));
    run(p, 0.3);
    CHECK(p.fractions()[1] == doctest::Approx(0.6));
    REQUIRE(p.accessible().state.range);
}

TEST_CASE("a circular indicator fits a wave of about 15 dp around its 48 dp circle and starts at 12 o'clock going clockwise") {
    CircularWavyProgressIndicator c({.progress = 0.3});
    c.set_origin({0.0, 0.0});
    CHECK(c.vertex_count() == 9);
    run(c, 0.6);
    const auto line = c.active_line();
    REQUIRE(line.size() > 4);
    const render::Point first = line.front();
    CHECK(first.y < 6.0);
    CHECK(line[3].x > first.x);
    const auto track = c.track_line();
    REQUIRE(track.size() > 4);
    const render::Point a = track.front();
    const render::Point b = line.back();
    CHECK(std::hypot(a.x - b.x, a.y - b.y) > 4.0);
}

TEST_CASE("an indeterminate circular indicator rotates and sweeps and hands off to a determinate one") {
    CircularWavyProgressIndicator c;
    CHECK(c.rotation_degrees() == doctest::Approx(90.0));
    run(c, 1.0);
    CHECK(c.sweep() > 0.1);
    c.set_progress(0.4);
    CHECK(c.handoff() == doctest::Approx(0.0));
    run(c, 0.7);
    CHECK(c.handoff() == doctest::Approx(1.0));
    CHECK(c.rotation_degrees() == doctest::Approx(0.0));
    CHECK(c.sweep() == doctest::Approx(0.4));
}

TEST_CASE("wavy indicators paint the indicator and track colors") {
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    LinearWavyProgressIndicator p({.progress = 0.05});
    p.set_origin({0.0, 0.0});
    thorvg::Canvas canvas(240, 10);
    canvas.clear(0xffffffff);
    p.paint(canvas, scheme);
    CHECK(canvas.pixel(200, 5) == scheme[color::Role::secondary_container]);
    CHECK(canvas.pixel(5, 5) == scheme[color::Role::primary]);
}
