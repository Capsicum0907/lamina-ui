#include <doctest/doctest.h>

#include "timer/layout.hpp"

using namespace lamina::examples::timer;

namespace {

bool inside(const Box& b, double width, double height) {
    return b.x >= -1e-9 && b.y >= -1e-9 && b.x + b.width <= width + 1e-9 && b.y + b.height <= height + 1e-9;
}

double center_x(const Box& b) {
    return b.x + b.width / 2.0;
}

double center_y(const Box& b) {
    return b.y + b.height / 2.0;
}

}

TEST_CASE("the portrait entry stacks the readout, the presets, a 3 x 4 keypad of circles and the start key") {
    TimerLayout layout;
    const TimerFrame f = layout.place(400.0, 780.0);
    CHECK(f.arrangement == Arrangement::portrait);
    const EntryFrame& e = f.entry;
    const double d = e.keys[0].width;
    CHECK(d > 0.0);
    for (const Box& k : e.keys) {
        CHECK(k.width == doctest::Approx(d));
        CHECK(k.height == doctest::Approx(d));
        CHECK(inside(k, 400.0, 780.0));
    }
    CHECK(e.keys[1].x - e.keys[0].x == doctest::Approx(d + 12.0));
    CHECK(e.keys[3].y - e.keys[0].y == doctest::Approx(d + 12.0));
    CHECK(center_x(e.keys[1]) == doctest::Approx(200.0));
    CHECK(e.start.y >= e.keys[11].y + d);
    CHECK(center_x(e.start) == doctest::Approx(200.0));
    CHECK(e.start.y + e.start.height == doctest::Approx(780.0 - 16.0));
    CHECK(e.presets[0].y + e.presets[0].height <= e.keys[0].y);
    CHECK(e.presets[0].x == doctest::Approx(e.keys[0].x));
    CHECK(e.presets[3].x + e.presets[3].width == doctest::Approx(e.keys[2].x + d));
    CHECK(e.readout.y + e.readout.height <= e.presets[0].y);
    CHECK(e.readout.height >= layout.settings().readout_min_height - 1e-9);
}

TEST_CASE("keys stop growing at the cap and the keypad stays centered") {
    TimerLayout layout;
    const TimerFrame f = layout.place(700.0, 1400.0);
    CHECK(f.arrangement == Arrangement::portrait);
    CHECK(f.entry.keys[0].width == doctest::Approx(layout.settings().max_key));
    CHECK(center_x(f.entry.keys[1]) == doctest::Approx(350.0));
}

TEST_CASE("the landscape entry puts the keypad on the right and the readout, presets and start on the left") {
    TimerLayout layout;
    const TimerFrame f = layout.place(900.0, 500.0);
    CHECK(f.arrangement == Arrangement::landscape);
    const EntryFrame& e = f.entry;
    CHECK(e.keys[2].x + e.keys[2].width == doctest::Approx(900.0 - 16.0));
    CHECK(e.readout.x + e.readout.width <= e.keys[0].x);
    CHECK(e.start.x + e.start.width <= e.keys[0].x);
    CHECK(e.presets[3].x + e.presets[3].width <= e.keys[0].x);
    CHECK(e.readout.height <= layout.settings().readout_max_height + 1e-9);
    CHECK(e.presets[0].y == doctest::Approx(e.readout.y + e.readout.height + 12.0));
    CHECK(e.start.y == doctest::Approx(e.presets[0].y + e.presets[0].height + 12.0));
    CHECK((e.readout.y + e.start.y + e.start.height) / 2.0 == doctest::Approx(250.0));
    for (const Box& k : e.keys) {
        CHECK(inside(k, 900.0, 500.0));
    }
}

TEST_CASE("the running ring is the main element, centered, with the controls below it in portrait") {
    TimerLayout layout;
    const RunningFrame r = layout.place(400.0, 780.0).running;
    CHECK(r.ring.width == doctest::Approx(400.0 - 32.0));
    CHECK(center_x(r.ring) == doctest::Approx(200.0));
    CHECK(r.add.y >= r.ring.y + r.ring.height);
    CHECK(r.add.y == doctest::Approx(r.reset.y));
    CHECK(r.reset.width == doctest::Approx(r.reset.height));
    CHECK((r.add.x + r.reset.x + r.reset.width) / 2.0 == doctest::Approx(200.0));
    CHECK(center_x(r.time) == doctest::Approx(center_x(r.ring)));
    CHECK(center_x(r.toggle) == doctest::Approx(center_x(r.ring)));
    CHECK(center_y(r.time) < center_y(r.ring));
    CHECK(center_y(r.toggle) > center_y(r.ring));
    CHECK(r.toggle.y + r.toggle.height < r.ring.y + r.ring.height);
    CHECK(inside(r.reset, 400.0, 780.0));
}

TEST_CASE("in landscape the controls sit beside the ring") {
    TimerLayout layout;
    const RunningFrame r = layout.place(900.0, 500.0).running;
    CHECK(r.ring.height == doctest::Approx(500.0 - 32.0));
    CHECK(r.add.x >= r.ring.x + r.ring.width);
    CHECK(r.reset.y >= r.add.y + r.add.height);
    CHECK((r.add.y + r.reset.y + r.reset.height) / 2.0 == doctest::Approx(center_y(r.ring)));
    CHECK((r.ring.x + r.add.x + r.add.width) / 2.0 == doctest::Approx(450.0));
}

TEST_CASE("the arrangement switches with hysteresis") {
    TimerLayout layout;
    CHECK(layout.place(400.0, 780.0).arrangement == Arrangement::portrait);
    CHECK(layout.place(550.0, 500.0).arrangement == Arrangement::portrait);
    CHECK(layout.place(610.0, 500.0).arrangement == Arrangement::landscape);
    CHECK(layout.place(550.0, 500.0).arrangement == Arrangement::landscape);
    CHECK(layout.place(495.0, 500.0).arrangement == Arrangement::portrait);
}
