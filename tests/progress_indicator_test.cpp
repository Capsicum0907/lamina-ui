#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <numbers>

#include "lamina/render/path.hpp"
#include "lamina/widgets/progress_indicator.hpp"
#include "lamina/widgets/protocol.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using namespace lamina::widgets;

static_assert(Paintable<CircularProgressIndicator> && Steppable<CircularProgressIndicator> &&
              SizeSettable<CircularProgressIndicator> && !HandlesPointer<CircularProgressIndicator>);

namespace {

constexpr double side = 200.0;
constexpr double thickness = 12.0;

CircularProgressIndicator make() {
    CircularProgressIndicator p({.diameter = side, .thickness = thickness, .track_thickness = thickness, .gap = 6.0,
                                 .amplitude = 4.0, .wavelength = 40.0});
    return p;
}

color::Argb at_angle(thorvg::Canvas& canvas, const CircularProgressIndicator& p, double degrees) {
    const double r = side / 2.0 - thickness / 2.0 - 4.0;
    const double a = degrees * std::numbers::pi / 180.0 - std::numbers::pi / 2.0;
    return canvas.pixel(static_cast<int>(std::lround(p.width() / 2.0 + r * std::cos(a))),
                        static_cast<int>(std::lround(p.height() / 2.0 + r * std::sin(a))));
}

}

TEST_CASE("stroke_path outlines a line with round caps") {
    thorvg::Canvas canvas(100, 40);
    canvas.clear(0xffffffff);
    const std::array<render::Point, 2> line{{{20.0, 20.0}, {80.0, 20.0}}};
    canvas.fill_path(render::stroke_path(line, 10.0), 0xff000000);
    CHECK(canvas.pixel(50, 20) == 0xff000000);
    CHECK(canvas.pixel(50, 17) == 0xff000000);
    CHECK(canvas.pixel(50, 28) == 0xffffffff);
    CHECK(canvas.pixel(17, 20) == 0xff000000);
    CHECK(canvas.pixel(83, 20) == 0xff000000);
    CHECK(canvas.pixel(15, 15) == 0xffffffff);
    CHECK(render::stroke_path(std::span<const render::Point>{}, 10.0).empty());
}

TEST_CASE("the indicator runs clockwise from the top and the track fills the rest past a gap") {
    auto p = make();
    p.set_wavy(false);
    p.set_progress(0.25);
    settle(p);
    thorvg::Canvas canvas(static_cast<int>(side), static_cast<int>(side));
    canvas.clear(0xffffffff);
    p.paint(canvas, scheme());
    const color::Argb indicator = scheme()[tokens::circular_progress_tokens.indicator];
    const color::Argb track = scheme()[tokens::circular_progress_tokens.track];
    CHECK(at_angle(canvas, p, 45.0) == indicator);
    CHECK(at_angle(canvas, p, 180.0) == track);
    CHECK(at_angle(canvas, p, 270.0) == track);
    CHECK(at_angle(canvas, p, 90.0 + 5.7) == 0xffffffff);
    CHECK(at_angle(canvas, p, 360.0 - 5.7) == 0xffffffff);
}

TEST_CASE("an empty indicator leaves a closed track and a full one hides it") {
    auto p = make();
    p.set_wavy(false);
    thorvg::Canvas canvas(static_cast<int>(side), static_cast<int>(side));
    const color::Argb indicator = scheme()[tokens::circular_progress_tokens.indicator];
    const color::Argb track = scheme()[tokens::circular_progress_tokens.track];
    p.set_progress(0.0);
    canvas.clear(0xffffffff);
    p.paint(canvas, scheme());
    CHECK(count_color(canvas, indicator) == 0);
    CHECK(at_angle(canvas, p, 0.0) == track);
    p.set_progress(1.0);
    canvas.clear(0xffffffff);
    p.paint(canvas, scheme());
    CHECK(count_color(canvas, track) == 0);
    CHECK(at_angle(canvas, p, 180.0) == indicator);
}

TEST_CASE("the wave grows inside the progress range, keeps moving, and settles when flattened") {
    auto p = make();
    p.set_progress(0.5);
    CHECK(p.animating());
    settle(p);
    CHECK(p.amplitude() == doctest::Approx(1.0));
    CHECK(p.animating());
    p.set_wavy(false);
    settle(p);
    CHECK(p.amplitude() == doctest::Approx(0.0));
    CHECK_FALSE(p.animating());

    p.set_wavy(true);
    p.set_progress(0.97);
    settle(p);
    CHECK(p.amplitude() == doctest::Approx(0.0));
    CHECK_FALSE(p.animating());
    p.set_progress(0.05);
    settle(p);
    CHECK_FALSE(p.animating());
}

TEST_CASE("ease_progress glides to a new value while set_progress follows at once") {
    auto p = make();
    p.set_wavy(false);
    p.set_progress(0.2);
    CHECK(p.progress() == doctest::Approx(0.2));
    p.ease_progress(0.8);
    CHECK(p.progress() == doctest::Approx(0.2));
    CHECK(p.animating());
    p.step(frame_seconds);
    CHECK(p.progress() > 0.2);
    CHECK(p.progress() < 0.8);
    p.set_progress(0.7);
    settle(p);
    CHECK(p.progress() == doctest::Approx(0.7));
    CHECK_FALSE(p.animating());
}

TEST_CASE("set_colors cross-fades to the new roles") {
    auto p = make();
    p.set_wavy(false);
    p.set_progress(1.0);
    p.set_colors({color::Role::error, color::Role::error_container});
    CHECK(p.animating());
    settle(p);
    CHECK_FALSE(p.animating());
    thorvg::Canvas canvas(static_cast<int>(side), static_cast<int>(side));
    canvas.clear(0xffffffff);
    p.paint(canvas, scheme());
    CHECK(at_angle(canvas, p, 180.0) == scheme()[color::Role::error]);
}
