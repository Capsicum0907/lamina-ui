#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/loading_indicator.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using widgets::LoadingIndicator;

namespace {

struct Extent {
    double left;
    double top;
    double right;
    double bottom;
};

Extent extent(const render::Path& p) {
    Extent e{1e9, 1e9, -1e9, -1e9};
    for (const render::Point& q : p.points()) {
        e.left = std::min(e.left, q.x);
        e.top = std::min(e.top, q.y);
        e.right = std::max(e.right, q.x);
        e.bottom = std::max(e.bottom, q.y);
    }
    return e;
}

}

static_assert(widgets::Sized<LoadingIndicator>);
static_assert(widgets::Steppable<LoadingIndicator>);

TEST_CASE("loading indicator tokens follow LoadingIndicatorTokens.kt and LoadingIndicator.kt") {
    const auto& t = tokens::loading_indicator_tokens;
    CHECK(t.container_size == 48.0);
    CHECK(t.active_size == 38.0);
    CHECK(t.indicator == color::Role::primary);
    CHECK(t.contained_indicator == color::Role::on_primary_container);
    CHECK(t.contained_container == color::Role::primary_container);
    CHECK(t.morph_spring.damping_ratio == doctest::Approx(0.6));
    CHECK(t.morph_spring.stiffness == doctest::Approx(200.0));
    CHECK(t.morph_interval_seconds == doctest::Approx(0.65));
    CHECK(t.rotation_period_seconds == doctest::Approx(4.666));
    CHECK(tokens::loading_indicator_indeterminate_shapes.front() == shapes::MaterialShape::soft_burst);
    CHECK(tokens::loading_indicator_indeterminate_shapes.back() == shapes::MaterialShape::oval);
}

TEST_CASE("the indeterminate sequence closes into a loop and the determinate one runs once") {
    CHECK(widgets::loading_indicator_morphs(false).size() == 7);
    CHECK(widgets::loading_indicator_morphs(true).size() == 1);
    CHECK(widgets::loading_indicator_scale(false) > 0.5);
    CHECK(widgets::loading_indicator_scale(false) <= 38.0 / 48.0);
}

TEST_CASE("the morph spring finishes when Compose's estimate says, about 0.3 s for a 0.1 threshold") {
    CHECK(widgets::spring_finish_seconds(tokens::loading_indicator_tokens.morph_spring, 0.1) == doctest::Approx(0.2977).epsilon(0.01));
}

TEST_CASE("an indeterminate indicator moves to the next shape every 650 ms and adds a quarter turn") {
    LoadingIndicator li;
    CHECK(li.morph_index() == 0);
    CHECK(li.animating());
    double peak = 0.0;
    for (int i = 0; i < 18; ++i) {
        li.step(frame_seconds);
        peak = std::max(peak, li.morph_progress());
    }
    CHECK(peak > 1.0);
    CHECK(li.morph_index() == 1);
    CHECK(li.morph_progress() == 0.0);
    for (int i = 0; i < 24; ++i) {
        li.step(frame_seconds);
    }
    CHECK(li.morph_index() == 1);
    CHECK(li.morph_progress() > 0.0);
    for (int i = 0; i < 7 * 39; ++i) {
        li.step(frame_seconds);
    }
    CHECK(li.morph_index() == 1);
}

TEST_CASE("the rotation grows continuously across a shape change") {
    LoadingIndicator li;
    double last = li.rotation_degrees();
    for (int i = 0; i < 120; ++i) {
        li.step(frame_seconds);
        const double now = li.rotation_degrees();
        double d = now - last;
        if (d < -180.0) {
            d += 360.0;
        }
        CHECK(d > -12.0);
        CHECK(d < 40.0);
        last = now;
    }
}

TEST_CASE("the indicator stays inside its 48 dp container") {
    LoadingIndicator li;
    li.set_origin({10.0, 20.0});
    for (int i = 0; i < 300; ++i) {
        li.step(frame_seconds);
        const Extent e = extent(li.indicator_path());
        CHECK(e.left >= 10.0 - 2.0);
        CHECK(e.top >= 20.0 - 2.0);
        CHECK(e.right <= 58.0 + 2.0);
        CHECK(e.bottom <= 68.0 + 2.0);
    }
}

TEST_CASE("a determinate indicator turns back half a turn as it fills and reports its range") {
    LoadingIndicator li({.progress = 0.5, .accessible_name = "Loading"});
    CHECK(li.determinate());
    CHECK_FALSE(li.animating());
    CHECK(li.rotation_degrees() == doctest::Approx(-90.0));
    CHECK(li.morph_progress() == doctest::Approx(0.5));
    const auto node = li.accessible();
    CHECK(node.role == widgets::AccessibleRole::progress_indicator);
    REQUIRE(node.state.range);
    CHECK(node.state.range->value == doctest::Approx(0.5));
    li.set_progress(1.0);
    CHECK(li.morph_progress() == doctest::Approx(1.0));
    CHECK(li.rotation_degrees() == doctest::Approx(-180.0));
    li.set_progress(std::nullopt);
    CHECK(li.animating());
    CHECK_FALSE(li.accessible().state.range);
}

TEST_CASE("a contained indicator paints its primary-container circle under the shape") {
    const color::ColorScheme scheme = color::spec_2021().build(color::default_scheme_options);
    LoadingIndicator li({.contained = true});
    thorvg::Canvas canvas(48, 48);
    canvas.clear(0xffffffff);
    li.paint(canvas, scheme);
    CHECK(canvas.pixel(24, 2) == scheme[color::Role::primary_container]);
    CHECK(canvas.pixel(24, 24) == scheme[color::Role::on_primary_container]);
    CHECK(canvas.pixel(1, 1) == 0xffffffff);
}
