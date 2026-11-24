#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/render/video.hpp"
#include "lamina_thorvg/canvas.hpp"

using namespace lamina;

namespace {

constexpr int video_w = 64;
constexpr int video_h = 36;

class AcceptVideo final : public render::RetainedImageSink {
public:
    bool accepts(const render::Image&) const override { return true; }
    bool accepts_video(const render::VideoFrame&) const override { return videos; }
    bool videos = true;
};

render::VideoFrame flat(std::uint8_t y, std::uint8_t cb, std::uint8_t cr, std::uint64_t generation,
                        std::shared_ptr<const render::ImageIdentity> surface) {
    render::VideoFrame f;
    f.width = video_w;
    f.height = video_h;
    f.layout = render::VideoLayout::nv12;
    f.y_pitch = video_w;
    f.uv_pitch = video_w;
    f.y.assign(static_cast<std::size_t>(video_w * video_h), y);
    f.u.resize(static_cast<std::size_t>(video_w * video_h / 2));
    for (std::size_t i = 0; i < f.u.size(); i += 2) {
        f.u[i] = cb;
        f.u[i + 1] = cr;
    }
    f.surface = std::move(surface);
    f.generation = generation;
    return f;
}

diagnostics::FrameCounts frame(thorvg::Canvas& canvas, diagnostics::FrameProbe& probe, const render::VideoFrame& video) {
    probe.begin_frame(0.0);
    canvas.clear(0xff000000u);
    canvas.draw_video(video, {0.0, 20.0, 320.0, 180.0});
    canvas.fill_rect({0.0, 0.0, 320.0, 40.0}, 0xb3000000u);
    canvas.fill_rect({0.0, 180.0, 320.0, 40.0}, 0xb3000000u);
    canvas.finish();
    const diagnostics::FrameCounts counts = probe.counts();
    probe.end_frame();
    return counts;
}

}

TEST_CASE("a video frame turns into the RGB its matrix and range give, and a short frame is not valid") {
    const auto s = render::video_surface();
    const render::Image red = render::video_image(flat(63, 102, 240, 1, s));
    REQUIRE(red.width == video_w);
    const std::uint32_t p = red.pixels[5];
    CHECK((p >> 24) == 0xFFu);
    CHECK(((p >> 16) & 0xFF) >= 252);
    CHECK(((p >> 8) & 0xFF) <= 3);
    render::VideoFrame wide = flat(235, 128, 128, 1, s);
    wide.full_range = true;
    CHECK(((render::video_image(wide).pixels[0] >> 8) & 0xFF) == 235);
    render::VideoFrame bad = flat(16, 128, 128, 1, s);
    bad.y.resize(10);
    CHECK_FALSE(bad.valid());
    CHECK(render::video_image(bad).pixels.empty());
}

TEST_CASE("a video drawn on the GPU leaves layer 0 alone when only its frame changes, with bars over it in the layer above") {
    thorvg::Canvas canvas(320, 220);
    AcceptVideo sink;
    canvas.set_retained_images(&sink);
    diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    const auto surface = render::video_surface();
    frame(canvas, probe, flat(100, 128, 128, 1, surface));
    canvas.take_changed_pixels();
    const diagnostics::FrameCounts next = frame(canvas, probe, flat(180, 90, 200, 2, surface));
    CHECK(next.repaint_pixels == 0);
    CHECK(next.repaint_tiles == 0);
    CHECK(next.gpu_images == 1);
    REQUIRE(canvas.retained_draws().size() == 1);
    REQUIRE(canvas.retained_draws()[0].size() == 1);
    CHECK(canvas.retained_draws()[0][0].video != nullptr);
    CHECK(canvas.retained_draws()[0][0].image == nullptr);
    CHECK_FALSE(canvas.upper_layers().empty());
    canvas.set_probe(nullptr);
}

TEST_CASE("a video drawn on the CPU repaints when its frame changes and not when it stays") {
    thorvg::Canvas canvas(320, 220);
    diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    const auto surface = render::video_surface();
    frame(canvas, probe, flat(100, 128, 128, 1, surface));
    const diagnostics::FrameCounts same = frame(canvas, probe, flat(100, 128, 128, 1, surface));
    CHECK(same.repaint_pixels == 0);
    const diagnostics::FrameCounts changed = frame(canvas, probe, flat(200, 128, 128, 2, surface));
    CHECK(changed.repaint_pixels > 0);
    CHECK(changed.gpu_images == 0);
    const std::uint32_t mid = canvas.pixel(160, 110);
    CHECK(((mid >> 8) & 0xFF) > 200);
    canvas.set_probe(nullptr);
}

TEST_CASE("a window that cannot take a video gets its frames as images, one per frame") {
    thorvg::Canvas canvas(320, 220);
    AcceptVideo sink;
    sink.videos = false;
    canvas.set_retained_images(&sink);
    const auto surface = render::video_surface();
    canvas.clear(0xff000000u);
    const render::VideoFrame f = flat(100, 128, 128, 1, surface);
    canvas.draw_video(f, {0.0, 0.0, 320.0, 180.0});
    canvas.finish();
    REQUIRE(canvas.retained_draws().size() == 1);
    const render::RetainedDraw& d = canvas.retained_draws()[0][0];
    CHECK(d.video == nullptr);
    REQUIRE(d.image != nullptr);
    CHECK(d.image->identity != nullptr);
    CHECK(d.image->width == video_w);
}

TEST_CASE("a video drawn at a quarter of its size still goes to the GPU, where an image would not") {
    thorvg::Canvas canvas(320, 220);
    AcceptVideo sink;
    canvas.set_retained_images(&sink);
    canvas.clear(0xff000000u);
    const render::VideoFrame f = flat(100, 128, 128, 1, render::video_surface());
    canvas.draw_video(f, {0.0, 0.0, video_w / 4.0, video_h / 4.0});
    canvas.finish();
    REQUIRE(canvas.retained_draws().size() == 1);
    CHECK(canvas.retained_draws()[0][0].video != nullptr);
}

TEST_CASE("an ARGB video frame keeps its straight alpha, goes to the GPU like a YUV one, and blends on the CPU without one") {
    render::VideoFrame f;
    f.width = video_w;
    f.height = video_h;
    f.layout = render::VideoLayout::argb;
    f.argb.assign(static_cast<std::size_t>(video_w * video_h), 0x80FF0000u);
    f.surface = render::video_surface();
    f.generation = 1;
    REQUIRE(f.valid());
    CHECK(f.bytes() == static_cast<std::size_t>(video_w * video_h) * sizeof(std::uint32_t));
    const render::Image image = render::video_image(f);
    REQUIRE(image.pixels.size() == f.argb.size());
    CHECK(image.pixels[7] == 0x80FF0000u);
    render::VideoFrame short_frame = f;
    short_frame.argb.resize(10);
    CHECK_FALSE(short_frame.valid());

    thorvg::Canvas gpu(320, 220);
    AcceptVideo sink;
    gpu.set_retained_images(&sink);
    gpu.clear(0xff000000u);
    gpu.draw_video(f, {0.0, 0.0, 320.0, 180.0});
    gpu.finish();
    REQUIRE(gpu.retained_draws().size() == 1);
    CHECK(gpu.retained_draws()[0][0].video == &f);

    thorvg::Canvas cpu(320, 220);
    cpu.clear(0xff0000ffu);
    cpu.draw_video(f, {0.0, 0.0, 320.0, 180.0});
    cpu.finish();
    const std::uint32_t mid = cpu.pixel(160, 90);
    CHECK(((mid >> 16) & 0xFF) >= 126);
    CHECK(((mid >> 16) & 0xFF) <= 130);
    CHECK((mid & 0xFF) >= 125);
    CHECK((mid & 0xFF) <= 129);
}
