#include <doctest/doctest.h>

#include <string>
#include <utility>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

namespace {

constexpr int partway_frames = 3;

struct Log {
    std::vector<std::string> paints;
    std::vector<std::pair<std::string, double>> steps;
};

class Recorder {
public:
    Recorder(std::string name, Log& log, bool moving = false) : name_(std::move(name)), log_(log), moving_(moving) {}
    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    void step(double dt_seconds) { log_.steps.emplace_back(name_, dt_seconds); }
    void paint(render::Canvas&, const color::ColorScheme&) const { log_.paints.push_back(name_); }
    bool animating() const { return moving_; }
    void set_moving(bool moving) { moving_ = moving; }

private:
    std::string name_;
    Log& log_;
    bool moving_;
};

}

TEST_CASE("an empty widget set is not animating and paints nothing") {
    widgets::WidgetSet set;
    thorvg::Canvas canvas(8, 8);
    CHECK(set.empty());
    CHECK(set.size() == 0);
    CHECK_FALSE(set.animating());
    set.step(frame_seconds);
    set.paint(canvas, scheme());
}

TEST_CASE("a widget set steps every member with the same time step") {
    Log log;
    Recorder a("a", log);
    widgets::WidgetSet set;
    set.add(a);
    set.emplace<Recorder>("b", log);
    set.step(0.25);
    REQUIRE(log.steps.size() == 2);
    CHECK(log.steps[0] == std::pair<std::string, double>{"a", 0.25});
    CHECK(log.steps[1] == std::pair<std::string, double>{"b", 0.25});
}

TEST_CASE("a widget set paints its members in the order they were added") {
    Log log;
    Recorder first("first", log);
    Recorder third("third", log);
    widgets::WidgetSet set;
    set.add(first);
    set.emplace<Recorder>("second", log);
    set.add(third);
    set.emplace<Recorder>("fourth", log);
    CHECK(set.size() == 4);
    thorvg::Canvas canvas(8, 8);
    set.paint(canvas, scheme());
    CHECK(log.paints == std::vector<std::string>{"first", "second", "third", "fourth"});
}

TEST_CASE("a widget set is animating while any member is") {
    Log log;
    Recorder referenced("referenced", log);
    widgets::WidgetSet set;
    set.add(referenced);
    Recorder& owned = set.emplace<Recorder>("owned", log);
    CHECK_FALSE(set.animating());
    owned.set_moving(true);
    CHECK(set.animating());
    referenced.set_moving(true);
    CHECK(set.animating());
    owned.set_moving(false);
    CHECK(set.animating());
    referenced.set_moving(false);
    CHECK_FALSE(set.animating());
}

TEST_CASE("an owned member keeps its address as the set grows") {
    Log log;
    widgets::WidgetSet set;
    Recorder& first = set.emplace<Recorder>("first", log);
    for (int i = 0; i < 64; ++i) {
        set.emplace<Recorder>(std::to_string(i), log);
    }
    first.set_moving(true);
    CHECK(set.animating());
}

TEST_CASE("a widget set steps and paints real widgets as they would be on their own") {
    const widgets::ButtonOptions options{"Send", tokens::ButtonStyle::filled, tokens::ButtonSize::small, tokens::ButtonShape::round};
    widgets::Button alone(options, fonts());
    widgets::WidgetSet set;
    widgets::Button& member = set.emplace<widgets::Button>(options, fonts());
    for (auto* b : {&alone, &member}) {
        b->set_origin({4.0, 4.0});
        b->handle({input::PointerAction::down, 10.0, 10.0});
    }
    CHECK(set.animating());
    for (int i = 0; i < partway_frames; ++i) {
        alone.step(frame_seconds);
        set.step(frame_seconds);
    }
    CHECK(alone.animating());
    CHECK(set.animating());
    thorvg::Canvas a(160, 64);
    thorvg::Canvas b(160, 64);
    a.clear(0);
    b.clear(0);
    alone.paint(a, scheme());
    set.paint(b, scheme());
    REQUIRE(a.width() == b.width());
    REQUIRE(a.height() == b.height());
    int differing = 0;
    int painted = 0;
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            differing += a.pixel(x, y) != b.pixel(x, y) ? 1 : 0;
            painted += a.pixel(x, y) != 0 ? 1 : 0;
        }
    }
    CHECK(painted > 0);
    CHECK(differing == 0);
}
