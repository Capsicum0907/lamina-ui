#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#include "lamina/diagnostics/frame_stats.hpp"

#include "lamina_thorvg/canvas.hpp"

using namespace lamina::render;
using lamina::color::Argb;
using lamina::render::Image;

namespace {

constexpr Argb white = 0xffffffff;
constexpr Argb red = 0xffff0000;

int channel(Argb c, int shift) {
    return static_cast<int>((c >> shift) & 0xff);
}

bool near(Argb a, Argb b, int tolerance) {
    for (int shift : {0, 8, 16, 24}) {
        const int d = channel(a, shift) - channel(b, shift);
        if (d > tolerance || d < -tolerance) {
            return false;
        }
    }
    return true;
}

}

TEST_CASE("fit_radii scales radii down to fit the rectangle") {
    const Rect r{0, 0, 100, 40};
    const auto fitted = fit_radii(r, uniform_radii(30));
    CHECK(fitted.top_left == doctest::Approx(20));
    CHECK(fitted.bottom_right == doctest::Approx(20));
    const auto untouched = fit_radii(r, uniform_radii(10));
    CHECK(untouched.top_left == doctest::Approx(10));
}

TEST_CASE("transform stack composes translate and scale") {
    lamina::thorvg::Canvas canvas(4, 4);
    canvas.save();
    canvas.scale(2.0);
    canvas.translate(3.0, 1.0);
    const Point p = canvas.transform().apply({1.0, 1.0});
    CHECK(p.x == doctest::Approx(8.0));
    CHECK(p.y == doctest::Approx(4.0));
    canvas.restore();
    CHECK(canvas.transform().apply({1.0, 1.0}).x == doctest::Approx(1.0));
}

TEST_CASE_TEMPLATE("rounded rectangles cover the inside and leave the corners", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(100, 100);
    canvas.clear(white);
    canvas.fill_rounded_rect({10, 10, 80, 80}, uniform_radii(20), red);

    CHECK(canvas.pixel(50, 50) == red);
    CHECK(canvas.pixel(50, 11) == red);
    CHECK(canvas.pixel(11, 50) == red);
    CHECK(canvas.pixel(12, 12) == white);
    CHECK(canvas.pixel(87, 87) == white);
    CHECK(canvas.pixel(5, 5) == white);

    const Argb edge = canvas.pixel(15, 15);
    CHECK(edge != red);
    CHECK(edge != white);
}

TEST_CASE_TEMPLATE("translucent fills blend over what is below", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(10, 10);
    canvas.clear(white);
    canvas.fill_rounded_rect({0, 0, 10, 10}, uniform_radii(0), 0x80000000);
    CHECK(near(canvas.pixel(5, 5), 0xff7f7f7f, 2));
}

TEST_CASE_TEMPLATE("scale maps dp to pixels", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(40, 40);
    canvas.clear(white);
    canvas.scale(2.0);
    canvas.fill_rounded_rect({5, 5, 10, 10}, uniform_radii(0), red);
    CHECK(canvas.pixel(11, 11) == red);
    CHECK(canvas.pixel(29, 29) == red);
    CHECK(canvas.pixel(31, 31) == white);
}

TEST_CASE_TEMPLATE("fill_rect paints the same pixels as a rectangle path", CanvasType, lamina::thorvg::Canvas) {
    const Rect r{3.3, 4.7, 17.45, 9.6};
    CanvasType by_rect(48, 48);
    CanvasType by_path(48, 48);
    for (CanvasType* c : {&by_rect, &by_path}) {
        c->clear(white);
        c->translate(0.4, 0.15);
        c->scale(1.75);
    }
    by_rect.fill_rect(r, 0xc0ff0000);
    by_path.fill_path(rounded_rect_path(r, uniform_radii(0.0)), 0xc0ff0000);
    int differing = 0;
    for (int y = 0; y < 48; ++y) {
        for (int x = 0; x < 48; ++x) {
            differing += by_rect.pixel(x, y) != by_path.pixel(x, y);
        }
    }
    CHECK(differing == 0);
    const Argb edge = by_rect.pixel(6, 20);
    CHECK(edge != white);
    CHECK(edge != by_rect.pixel(20, 20));
}

TEST_CASE("point operators, length and unit") {
    const Point a{1.0, 2.0};
    const Point b{4.0, 6.0};
    CHECK(length(b - a) == 5.0);
    const Point u = unit(b - a);
    CHECK(u.x == doctest::Approx(0.6));
    CHECK(u.y == doctest::Approx(0.8));
    CHECK(unit({0.0, 0.0}).x == 0.0);
    const Point m = a + b * 0.5 - Point{1.0, 1.0};
    CHECK(m.x == 2.0);
    CHECK(m.y == 4.0);
    CHECK(left_normal({1.0, 0.0}).y == 1.0);
    CHECK(cross({1.0, 0.0}, {0.0, 1.0}) == 1.0);
    CHECK((-a / 2.0).y == -1.0);
    const Rect c = circle_rect({10.0, 10.0}, 4.0);
    CHECK(c.x == 6.0);
    CHECK(c.width == 8.0);
}

TEST_CASE_TEMPLATE("shadows fall in the offset direction", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(120, 120);
    canvas.clear(white);
    const Path shape = rounded_rect_path({30, 30, 60, 60}, uniform_radii(8));
    canvas.fill_shadow(shape, {0.0, 8.0, 4.0, 0xff000000});
    canvas.fill_path(shape, red);
    CHECK(canvas.pixel(60, 60) == red);
    const Argb below = canvas.pixel(60, 93);
    const Argb above = canvas.pixel(60, 27);
    CAPTURE(below);
    CAPTURE(above);
    CHECK(channel(below, 16) < 200);
    CHECK(channel(above, 16) > channel(below, 16));
}

TEST_CASE_TEMPLATE("an image is scaled into its rectangle and follows the transform", CanvasType, lamina::thorvg::Canvas) {
    const Image image{2, 1, {0xffff0000, 0xff0000ff}};
    CanvasType canvas(40, 40);
    canvas.clear(white);
    canvas.translate(2.0, 4.0);
    canvas.scale(2.0);
    canvas.draw_image(image, {4.0, 2.0, 12.0, 8.0});
    CHECK(near(canvas.pixel(12, 14), red, 8));
    CHECK(near(canvas.pixel(32, 14), 0xff0000ff, 8));
    CHECK(canvas.pixel(8, 14) == white);
    CHECK(canvas.pixel(36, 14) == white);
    CHECK(canvas.pixel(20, 6) == white);
    CHECK(canvas.pixel(20, 26) == white);
    CHECK(canvas.pixel(20, 22) != white);
}

TEST_CASE_TEMPLATE("an image's alpha is straight and its opacity fades it", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(10, 10);
    canvas.clear(white);
    canvas.draw_image({1, 1, {0x80000000}}, {0.0, 0.0, 5.0, 10.0});
    canvas.draw_image({1, 1, {0xff000000}}, {5.0, 0.0, 5.0, 10.0}, 0.5);
    CHECK(near(canvas.pixel(2, 5), 0xff7f7f7f, 3));
    CHECK(near(canvas.pixel(7, 5), 0xff7f7f7f, 3));
}

TEST_CASE_TEMPLATE("an image without pixels or at no opacity draws nothing", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(10, 10);
    canvas.clear(white);
    canvas.draw_image({}, {0.0, 0.0, 10.0, 10.0});
    canvas.draw_image({2, 2, {red}}, {0.0, 0.0, 10.0, 10.0});
    canvas.draw_image({1, 1, {red}}, {0.0, 0.0, 10.0, 10.0}, 0.0);
    CHECK(canvas.pixel(5, 5) == white);
}

TEST_CASE_TEMPLATE("a path clip keeps drawing inside the path, under the transform", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(60, 60);
    canvas.clear(white);
    canvas.translate(10.0, 10.0);
    canvas.scale(2.0);
    canvas.push_clip(circle_path({10.0, 10.0}, 10.0));
    canvas.fill_rect({0.0, 0.0, 20.0, 20.0}, red);
    canvas.draw_image({1, 1, {red}}, {0.0, 0.0, 20.0, 20.0});
    canvas.pop_clip();
    CHECK(canvas.pixel(30, 30) == red);
    CHECK(canvas.pixel(30, 12) == red);
    CHECK(canvas.pixel(12, 12) == white);
    CHECK(canvas.pixel(48, 48) == white);
    CHECK(canvas.pixel(5, 30) == white);
}

TEST_CASE_TEMPLATE("path and rectangle clips nest", CanvasType, lamina::thorvg::Canvas) {
    CanvasType canvas(40, 40);
    canvas.clear(white);
    canvas.push_clip(Rect{0.0, 0.0, 20.0, 40.0});
    canvas.push_clip(circle_path({20.0, 20.0}, 16.0));
    canvas.fill_rect({0.0, 0.0, 40.0, 40.0}, red);
    canvas.pop_clip();
    canvas.pop_clip();
    CHECK(canvas.pixel(10, 20) == red);
    CHECK(canvas.pixel(30, 20) == white);
    CHECK(canvas.pixel(2, 2) == white);
}

TEST_CASE("a blurred shadow is blurred once and drawn again from the cache when the shape only moves") {
    lamina::thorvg::Canvas canvas(160, 160);
    lamina::diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    const Shadow shadow{0.0, 6.0, 5.0, 0x80000000};
    auto frame = [&](double dy) {
        probe.begin_frame(0.0);
        canvas.clear(white);
        canvas.fill_shadow(rounded_rect_path({40, 30 + dy, 60, 40}, uniform_radii(10)), shadow);
        canvas.finish();
        const std::uint64_t made = probe.counts().shadow_renders;
        probe.end_frame();
        return made;
    };
    CHECK(frame(0.0) == 1);
    CHECK(frame(0.0) == 0);
    CHECK(frame(17.25) == 0);
    CHECK(frame(40.0) == 0);
    canvas.set_probe(nullptr);
}

TEST_CASE("a cached shadow moved by whole pixels lands on the same pixels moved") {
    lamina::thorvg::Canvas canvas(160, 160);
    const Shadow shadow{0.0, 6.0, 5.0, 0x80000000};
    canvas.clear(white);
    canvas.fill_shadow(rounded_rect_path({40, 30, 60, 40}, uniform_radii(10)), shadow);
    std::vector<Argb> first;
    for (int y = 0; y < 100; ++y) {
        first.push_back(canvas.pixel(70, y));
    }
    canvas.clear(white);
    canvas.fill_shadow(rounded_rect_path({40, 50, 60, 40}, uniform_radii(10)), shadow);
    for (int y = 0; y < 100; ++y) {
        CAPTURE(y);
        CHECK(canvas.pixel(70, y + 20) == first[static_cast<std::size_t>(y)]);
    }
}

TEST_CASE("a cached shadow is dark under its shape and fades out within its margin") {
    const Shadow shadow{0.0, 6.0, 5.0, 0x80000000};
    lamina::thorvg::Canvas canvas(160, 160);
    canvas.clear(white);
    canvas.fill_shadow(rounded_rect_path({40, 30, 60, 40}, uniform_radii(10)), shadow);
    const Argb under = canvas.pixel(70, 60);
    const Argb edge = canvas.pixel(70, 30 + 40 + 6 + 3 * 5);
    CAPTURE(under);
    CAPTURE(edge);
    CHECK(channel(under, 16) < 200);
    CHECK(near(edge, white, 3));
}

TEST_CASE("quick_reject follows the transform and the clips") {
    lamina::thorvg::Canvas canvas(100, 100);
    CHECK_FALSE(canvas.quick_reject({10, 10, 20, 20}));
    CHECK(canvas.quick_reject({120, 10, 20, 20}));
    CHECK_FALSE(canvas.quick_reject({120, 10, 20, 20}, 30.0));
    canvas.save();
    canvas.scale(2.0);
    CHECK(canvas.quick_reject({60, 10, 10, 10}));
    CHECK_FALSE(canvas.quick_reject({40, 10, 10, 10}));
    canvas.push_clip(Rect{0, 0, 20, 20});
    CHECK(canvas.quick_reject({25, 5, 10, 10}));
    CHECK_FALSE(canvas.quick_reject({15, 5, 10, 10}));
    canvas.push_clip(rounded_rect_path({10, 0, 30, 30}, uniform_radii(4)));
    CHECK(canvas.quick_reject({0, 0, 8, 8}));
    canvas.pop_clip();
    CHECK_FALSE(canvas.quick_reject({0, 0, 8, 8}));
    canvas.pop_clip();
    CHECK_FALSE(canvas.quick_reject({25, 5, 10, 10}));
    canvas.restore();
}

TEST_CASE("paths outside the canvas or the clip are counted as offscreen and not drawn") {
    lamina::thorvg::Canvas canvas(100, 100);
    lamina::diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    probe.begin_frame(0.0);
    canvas.clear(white);
    canvas.fill_rect({150, 10, 20, 20}, red);
    canvas.push_clip(Rect{0, 0, 50, 50});
    canvas.fill_rounded_rect({60, 60, 20, 20}, uniform_radii(4), red);
    canvas.fill_rect({10, 10, 20, 20}, red);
    canvas.pop_clip();
    canvas.fill_shadow(rounded_rect_path({110, 40, 20, 20}, uniform_radii(4)), {0.0, 2.0, 2.0, 0xff000000});
    const lamina::diagnostics::FrameCounts counts = probe.counts();
    probe.end_frame();
    canvas.set_probe(nullptr);
    CHECK(counts.paths == 3);
    CHECK(counts.offscreen_paths == 2);
    CHECK(counts.shadows == 1);
    CHECK(counts.shadow_renders == 0);
    CHECK(canvas.pixel(20, 20) == red);
    CHECK(canvas.pixel(70, 70) == white);
}

TEST_CASE("a shadow just outside the canvas is drawn when its blur reaches in") {
    lamina::thorvg::Canvas canvas(100, 100);
    canvas.clear(white);
    canvas.fill_shadow(rounded_rect_path({102, 30, 40, 40}, uniform_radii(4)), {0.0, 0.0, 4.0, 0xff000000});
    CHECK(channel(canvas.pixel(99, 50), 16) < 255);
}

namespace {

class AcceptAll final : public RetainedImageSink {
public:
    bool accepts(const Image&) const override { return true; }
};

Image solid(int w, int h, Argb c) {
    return Image{w, h, std::vector<Argb>(static_cast<std::size_t>(w * h), c), nullptr};
}

}

TEST_CASE("a retained image goes to the image layer and what overlaps it to a layer above") {
    lamina::thorvg::Canvas canvas(100, 100);
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    const Image photo = lamina::render::retained(solid(10, 10, red));
    canvas.clear(white);
    canvas.fill_rect({0, 0, 100, 20}, red);
    canvas.draw_image(photo, {10, 30, 50, 50});
    canvas.fill_rect({70, 30, 10, 10}, red);
    canvas.fill_rect({40, 70, 30, 10}, 0xff0000ffu);
    canvas.finish();
    REQUIRE(canvas.retained_draws().size() == 1);
    REQUIRE(canvas.retained_draws()[0].size() == 1);
    const RetainedDraw& d = canvas.retained_draws()[0][0];
    CHECK(d.image == &photo);
    CHECK(d.to_device.apply({10.0, 10.0}).x == doctest::Approx(60.0));
    CHECK(d.to_device.apply({10.0, 10.0}).y == doctest::Approx(80.0));
    CHECK(canvas.pixel(30, 50) == white);
    CHECK(canvas.pixel(75, 35) == red);
    const auto uppers = canvas.upper_layers();
    REQUIRE(uppers.size() == 1);
    CHECK(uppers[0].layer == 1);
    CHECK(uppers[0].x == 40);
    CHECK(uppers[0].y == 70);
    CHECK(uppers[0].pixels.width == 30);
    CHECK(uppers[0].pixels.height == 10);
    const auto* row = reinterpret_cast<const std::uint32_t*>(uppers[0].pixels.data + uppers[0].pixels.stride * 5);
    CHECK(row[5] == 0xff0000ffu);
    canvas.set_retained_images(nullptr);
}

TEST_CASE("an image under a path clip, without an identity or shrunk below half is drawn on the CPU") {
    lamina::thorvg::Canvas canvas(100, 100);
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    const Image photo = lamina::render::retained(solid(10, 10, red));
    const Image plain = solid(10, 10, red);
    canvas.clear(white);
    canvas.push_clip(rounded_rect_path({0, 0, 50, 50}, uniform_radii(8)));
    canvas.draw_image(photo, {10, 10, 30, 30});
    canvas.pop_clip();
    canvas.draw_image(plain, {60, 10, 30, 30});
    canvas.draw_image(photo, {60, 60, 4, 4});
    canvas.push_clip(Rect{0, 50, 50, 50});
    canvas.draw_image(photo, {10, 60, 30, 30});
    canvas.pop_clip();
    canvas.finish();
    CHECK(canvas.pixel(25, 25) == red);
    CHECK(canvas.pixel(75, 25) == red);
    CHECK(canvas.pixel(61, 61) == red);
    CHECK(canvas.pixel(25, 75) == white);
    REQUIRE(canvas.retained_draws().size() == 1);
    REQUIRE(canvas.retained_draws()[0].size() == 1);
    CHECK(canvas.retained_draws()[0][0].clip.y == doctest::Approx(50.0));
    CHECK(canvas.retained_draws()[0][0].clip.height == doctest::Approx(50.0));
    canvas.set_retained_images(nullptr);
}

TEST_CASE("without a sink a retained image is drawn on the CPU and clear forgets the layers") {
    lamina::thorvg::Canvas canvas(60, 60);
    const Image photo = lamina::render::retained(solid(10, 10, red));
    canvas.clear(white);
    canvas.draw_image(photo, {10, 10, 20, 20});
    CHECK(canvas.pixel(20, 20) == red);
    CHECK(canvas.retained_draws().empty());
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    canvas.clear(white);
    canvas.draw_image(photo, {10, 10, 20, 20});
    canvas.fill_rect({15, 15, 5, 5}, 0xff0000ffu);
    canvas.finish();
    CHECK(canvas.upper_layers().size() == 1);
    canvas.clear(white);
    canvas.finish();
    CHECK(canvas.retained_draws().empty());
    CHECK(canvas.upper_layers().empty());
    canvas.set_retained_images(nullptr);
}

TEST_CASE("clips carry into the layer above") {
    lamina::thorvg::Canvas canvas(100, 100);
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    const Image photo = lamina::render::retained(solid(10, 10, red));
    canvas.clear(white);
    canvas.push_clip(Rect{0, 0, 50, 100});
    canvas.draw_image(photo, {0, 0, 100, 100});
    canvas.fill_rect({20, 20, 60, 10}, 0xff0000ffu);
    canvas.pop_clip();
    canvas.finish();
    const auto uppers = canvas.upper_layers();
    REQUIRE(uppers.size() == 1);
    CHECK(uppers[0].x == 20);
    CHECK(uppers[0].pixels.width == 30);
    canvas.set_retained_images(nullptr);
}

TEST_CASE("prefetched images are listed for the window only while it takes retained images, until the next clear") {
    lamina::thorvg::Canvas canvas(20, 20);
    const Image photo = lamina::render::retained(solid(4, 4, red));
    const Image plain = solid(4, 4, red);
    canvas.clear(white);
    canvas.prefetch_image(photo);
    CHECK(canvas.prefetched_images().empty());
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    canvas.prefetch_image(photo);
    canvas.prefetch_image(plain);
    REQUIRE(canvas.prefetched_images().size() == 1);
    CHECK(canvas.prefetched_images()[0] == &photo);
    canvas.clear(white);
    CHECK(canvas.prefetched_images().empty());
    canvas.set_retained_images(nullptr);
}

TEST_CASE("an image drawn at under half its size stays on the CPU, unless the drawing allows minified images until its restore") {
    lamina::thorvg::Canvas canvas(100, 100);
    AcceptAll sink;
    canvas.set_retained_images(&sink);
    const Image photo = lamina::render::retained(solid(100, 100, red));
    const auto retained = [&canvas] {
        std::size_t n = 0;
        for (const auto& batch : canvas.retained_draws()) {
            n += batch.size();
        }
        return n;
    };
    canvas.clear(white);
    canvas.draw_image(photo, {0, 0, 30, 30});
    CHECK(retained() == 0);
    canvas.save();
    canvas.allow_minified_images();
    CHECK(canvas.minified_images_allowed());
    canvas.draw_image(photo, {40, 0, 30, 30});
    canvas.restore();
    CHECK_FALSE(canvas.minified_images_allowed());
    CHECK(retained() == 1);
    canvas.draw_image(photo, {0, 40, 30, 30});
    CHECK(retained() == 1);
    canvas.finish();
}
