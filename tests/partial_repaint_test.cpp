#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/render/image.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/bottom_sheet.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using render::PixelRect;

namespace {

class AcceptAll final : public render::RetainedImageSink {
public:
    bool accepts(const render::Image&) const override { return true; }
};

using Paint = std::function<void(render::Canvas&, int frame)>;
using Step = std::function<void(int frame)>;
using Background = std::function<color::Argb(int frame)>;

struct Size {
    int width;
    int height;
};

struct Report {
    int frames = 0;
    std::int64_t differing = 0;
    std::int64_t outside = 0;
    std::int64_t changed = 0;
    std::int64_t repainted = 0;
    std::int64_t full_area = 0;
    int empty_frames = 0;
    std::int64_t upper_stale = 0;
    std::int64_t upper_uploaded = 0;
    std::int64_t upper_area = 0;
    std::string first;
};

struct UpperTextures {
    int width = 0;
    int height = 0;
    std::vector<std::vector<std::uint32_t>> layers;

    void put(std::vector<std::uint32_t>& t, const render::UpperLayer& u, const PixelRect& r) {
        for (int y = r.y; y < r.y + r.height; ++y) {
            const auto* src = reinterpret_cast<const std::uint32_t*>(u.pixels.data + u.pixels.stride * (y - u.y)) + (r.x - u.x);
            std::memcpy(t.data() + static_cast<std::size_t>(y) * width + r.x, src, static_cast<std::size_t>(r.width) * 4);
        }
    }

    std::int64_t upload(const std::vector<render::UpperLayer>& uppers, int w, int h, Report& report) {
        if (w != width || h != height) {
            width = w;
            height = h;
            layers.clear();
        }
        std::int64_t stale = 0;
        for (const render::UpperLayer& u : uppers) {
            bool made = false;
            if (layers.size() < u.layer) {
                layers.resize(u.layer);
            }
            std::vector<std::uint32_t>& t = layers[u.layer - 1];
            if (t.empty()) {
                t.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0u);
                made = true;
            }
            if (made) {
                put(t, u, {u.x, u.y, u.pixels.width, u.pixels.height});
                report.upper_uploaded += static_cast<std::int64_t>(u.pixels.width) * u.pixels.height;
            } else {
                for (const PixelRect& c : u.changed) {
                    CHECK(c.x >= u.x);
                    CHECK(c.y >= u.y);
                    CHECK(c.x + c.width <= u.x + u.pixels.width);
                    CHECK(c.y + c.height <= u.y + u.pixels.height);
                    put(t, u, c);
                    report.upper_uploaded += c.area();
                }
            }
            report.upper_area += static_cast<std::int64_t>(u.pixels.width) * u.pixels.height;
            for (int y = 0; y < u.pixels.height; ++y) {
                const auto* row = reinterpret_cast<const std::uint32_t*>(u.pixels.data + u.pixels.stride * y);
                for (int x = 0; x < u.pixels.width; ++x) {
                    stale += t[static_cast<std::size_t>(y + u.y) * w + x + u.x] != row[x] ? 1 : 0;
                }
            }
        }
        return stale;
    }
};

std::vector<std::uint32_t> copy_of(const render::PixelView& v) {
    std::vector<std::uint32_t> out(static_cast<std::size_t>(v.width) * static_cast<std::size_t>(v.height));
    for (int y = 0; y < v.height; ++y) {
        std::memcpy(out.data() + static_cast<std::size_t>(y) * v.width, v.data + v.stride * y, static_cast<std::size_t>(v.width) * 4);
    }
    return out;
}

bool inside(const std::vector<PixelRect>& rects, int x, int y) {
    for (const PixelRect& r : rects) {
        if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height) {
            return true;
        }
    }
    return false;
}

bool same_draws(const render::Canvas& a, const render::Canvas& b) {
    const auto& da = a.retained_draws();
    const auto& db = b.retained_draws();
    if (da.size() != db.size()) {
        return false;
    }
    for (std::size_t i = 0; i < da.size(); ++i) {
        if (da[i].size() != db[i].size()) {
            return false;
        }
        for (std::size_t j = 0; j < da[i].size(); ++j) {
            const render::RetainedDraw& x = da[i][j];
            const render::RetainedDraw& y = db[i][j];
            if (x.image != y.image || x.opacity != y.opacity || x.to_device.tx != y.to_device.tx || x.to_device.ty != y.to_device.ty ||
                x.to_device.a != y.to_device.a || x.to_device.d != y.to_device.d || x.clip.x != y.clip.x || x.clip.y != y.clip.y ||
                x.clip.width != y.clip.width || x.clip.height != y.clip.height) {
                return false;
            }
        }
    }
    return true;
}

bool same_uppers(const render::Canvas& a, const render::Canvas& b) {
    const std::vector<render::UpperLayer> ua = a.upper_layers();
    const std::vector<render::UpperLayer> ub = b.upper_layers();
    if (ua.size() != ub.size()) {
        return false;
    }
    for (std::size_t i = 0; i < ua.size(); ++i) {
        if (ua[i].layer != ub[i].layer || ua[i].x != ub[i].x || ua[i].y != ub[i].y || ua[i].pixels.width != ub[i].pixels.width ||
            ua[i].pixels.height != ub[i].pixels.height || copy_of(ua[i].pixels) != copy_of(ub[i].pixels)) {
            return false;
        }
    }
    return true;
}

Report run(int frames, const std::function<Size(int)>& size, const Background& background, const Step& step, const Paint& paint,
           const render::RetainedImageSink* sink = nullptr) {
    Report report;
    const Size first = size(0);
    thorvg::Canvas tracked(first.width, first.height);
    tracked.set_retained_images(sink);
    REQUIRE(tracked.damage_tracking());
    std::vector<std::uint32_t> before;
    UpperTextures textures;
    for (int f = 0; f < frames; ++f) {
        const Size s = size(f);
        step(f);
        tracked.resize(s.width, s.height);
        tracked.clear(background(f));
        paint(tracked, f);
        const std::vector<std::uint32_t> got = copy_of(tracked.finish());
        const std::vector<PixelRect> rects = tracked.take_changed_pixels();
        report.upper_stale += textures.upload(tracked.upper_layers(), s.width, s.height, report);

        thorvg::Canvas full(s.width, s.height);
        full.set_damage_tracking(false);
        full.set_retained_images(sink);
        full.clear(background(f));
        paint(full, f);
        const std::vector<std::uint32_t> want = copy_of(full.finish());

        report.frames += 1;
        report.full_area += static_cast<std::int64_t>(s.width) * s.height;
        for (const PixelRect& r : rects) {
            report.repainted += r.area();
            CHECK(r.x >= 0);
            CHECK(r.y >= 0);
            CHECK(r.x + r.width <= s.width);
            CHECK(r.y + r.height <= s.height);
        }
        report.empty_frames += rects.empty() ? 1 : 0;
        const bool comparable = before.size() == got.size();
        for (int y = 0; y < s.height; ++y) {
            for (int x = 0; x < s.width; ++x) {
                const std::size_t i = static_cast<std::size_t>(y) * s.width + x;
                if (got[i] != want[i]) {
                    if (report.differing == 0) {
                        report.first = "frame " + std::to_string(f) + " at " + std::to_string(x) + "," + std::to_string(y);
                    }
                    report.differing += 1;
                }
                if (comparable && got[i] != before[i]) {
                    report.changed += 1;
                    if (!inside(rects, x, y)) {
                        if (report.outside == 0 && report.first.empty()) {
                            report.first = "outside the rects in frame " + std::to_string(f) + " at " + std::to_string(x) + "," + std::to_string(y);
                        }
                        report.outside += 1;
                    }
                }
            }
        }
        CHECK(same_uppers(tracked, full));
        CHECK(same_draws(tracked, full));
        before = got;
    }
    return report;
}

void expect_identical(const Report& r) {
    CAPTURE(r.first);
    CAPTURE(r.frames);
    CAPTURE(r.repainted);
    CAPTURE(r.full_area);
    CHECK(r.differing == 0);
    CHECK(r.outside == 0);
    CHECK(r.upper_stale == 0);
}

std::function<Size(int)> fixed(int w, int h) {
    return [w, h](int) { return Size{w, h}; };
}

std::vector<widgets::ListItemSpec> rows(std::size_t n) {
    std::vector<widgets::ListItemSpec> items;
    for (std::size_t i = 0; i < n; ++i) {
        items.push_back({"Row " + std::to_string(i + 1), i % 3 == 0 ? "Supporting text" : ""});
    }
    return items;
}

const color::ColorScheme& dark_scheme() {
    static const color::ColorScheme s = [] {
        color::SchemeOptions o = color::default_scheme_options;
        o.dark = true;
        return color::spec_2021().build(o);
    }();
    return s;
}

render::Image solid(int w, int h, color::Argb c) {
    return render::Image{w, h, std::vector<color::Argb>(static_cast<std::size_t>(w * h), c), nullptr};
}

render::Image gradient(int w, int h) {
    render::Image image{w, h, std::vector<color::Argb>(static_cast<std::size_t>(w * h)), nullptr};
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            image.pixels[static_cast<std::size_t>(y) * w + x] =
                0xff000000u | (static_cast<std::uint32_t>(x * 255 / w) << 16) | (static_cast<std::uint32_t>(y * 255 / h) << 8) | 0x40u;
        }
    }
    return image;
}

struct Buttons {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    layout::NodePtr node;

    explicit Buttons(int count) {
        std::vector<layout::NodePtr> nodes;
        for (int i = 0; i < count; ++i) {
            auto& b = widgets.emplace_with<widgets::Button>([](widgets::Button&, bool) {}, widgets::ButtonOptions{"Item " + std::to_string(i)},
                                                            fonts());
            focus.add(b);
            nodes.push_back(widgets::node(b));
        }
        auto column = layout::column(std::move(nodes));
        column->spacing(8.0);
        node = column;
    }

    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

Background opaque() {
    return [](int) { return scheme()[color::Role::surface]; };
}

}

TEST_CASE("damage tracking repaints a scrolling list exactly as a full repaint, at a fractional scale") {
    widgets::ListView list({rows(200), false, "Rows"}, fonts());
    constexpr double scale = 1.75;
    list.set_viewport({0.0, 64.0, 360.0, 280.0});
    const text::TextLine title = text::layout_line(fonts(), "Inbox", {22.0, 28.0, 400, 0.0, 0.0});
    const Report r = run(
        90, fixed(630, 1050), opaque(),
        [&](int f) {
            if (f > 0 && f < 18) {
                list.handle(input::WheelEvent{180.0, 200.0, 0.0, f < 10 ? 1.37 : -0.61});
            }
            list.step(frame_seconds);
        },
        [&](render::Canvas& c, int) {
            c.save();
            c.scale(scale);
            c.fill_rect({0.0, 0.0, 360.0, 64.0}, scheme()[color::Role::surface_container]);
            text::draw_line(c, fonts(), title, {16.0, 18.0}, scheme()[color::Role::on_surface]);
            list.paint(c, scheme());
            c.restore();
        });
    expect_identical(r);
    CHECK(r.repainted < r.full_area);
    CHECK(r.empty_frames > 0);
}

TEST_CASE("damage tracking repaints an animating button exactly, and only near it") {
    widgets::Button pressed({"Pressed", tokens::ButtonStyle::filled, tokens::ButtonSize::medium, tokens::ButtonShape::round}, fonts());
    widgets::Button still({"Still", tokens::ButtonStyle::outlined, tokens::ButtonSize::small, tokens::ButtonShape::square}, fonts());
    pressed.set_origin({24.0, 40.0});
    still.set_origin({24.0, 300.0});
    const Report r = run(
        30, fixed(400, 400), opaque(),
        [&](int f) {
            if (f == 2) {
                pressed.handle({input::PointerAction::down, 60.0, 60.0});
            }
            if (f == 14) {
                pressed.handle({input::PointerAction::up, 60.0, 60.0});
            }
            pressed.step(frame_seconds);
            still.step(frame_seconds);
        },
        [&](render::Canvas& c, int) {
            pressed.paint(c, scheme());
            still.paint(c, scheme());
        });
    expect_identical(r);
    CHECK(r.repainted < r.full_area / 2);
}

TEST_CASE("damage tracking repaints a dialog opening and closing over content, with its scrim and shadow, exactly") {
    widgets::ListView list({rows(40), false, "Rows"}, fonts());
    list.set_viewport({0.0, 0.0, 480.0, 640.0});
    widgets::Dialog dialog({"Discard draft?", "The message will be deleted from this device.", "Discard", "Cancel"}, fonts());
    const Report r = run(
        70, fixed(480, 640), opaque(),
        [&](int f) {
            if (f == 3) {
                dialog.open(layout::Size{480.0, 640.0});
            }
            if (f == 40) {
                dialog.close();
            }
            dialog.step(frame_seconds);
            list.step(frame_seconds);
        },
        [&](render::Canvas& c, int) {
            list.paint(c, scheme());
            dialog.paint(c, scheme());
        });
    expect_identical(r);
}

TEST_CASE("damage tracking repaints a modal bottom sheet sliding up over content exactly") {
    widgets::ListView list({rows(40), false, "Rows"}, fonts());
    list.set_viewport({0.0, 0.0, 400.0, 720.0});
    Buttons content(4);
    widgets::BottomSheet sheet({.content = content.slot()});
    sheet.set_area({0.0, 0.0, 400.0, 720.0});
    const Report r = run(
        60, fixed(400, 720), opaque(),
        [&](int f) {
            if (f == 2) {
                sheet.show();
            }
            if (f == 36) {
                sheet.hide();
            }
            sheet.step(frame_seconds);
        },
        [&](render::Canvas& c, int) {
            list.paint(c, scheme());
            sheet.paint(c, scheme());
        });
    expect_identical(r);
}

TEST_CASE("damage tracking keeps a retained image moving under UI content exact, with the layers above it") {
    AcceptAll sink;
    const render::Image photo = render::retained(gradient(64, 48));
    const render::Image avatar = solid(8, 8, 0xff3366ffu);
    const Report r = run(
        28, fixed(360, 480), opaque(), [](int) {},
        [&](render::Canvas& c, int f) {
            const double x = 20.0 + f * 7.3;
            const double y = 120.0 + std::sin(f * 0.4) * 30.0;
            c.fill_rect({0.0, 400.0, 360.0, 80.0}, scheme()[color::Role::surface_container]);
            c.draw_image(avatar, {300.0, 420.0, 40.0, 40.0});
            c.push_clip(render::Rect{0.0, 56.0, 360.0, 344.0});
            c.draw_image(photo, {x, y, 192.0, 144.0}, f % 5 == 0 ? 0.6 : 1.0);
            c.pop_clip();
            c.fill_rect({0.0, 0.0, 360.0, 56.0}, scheme()[color::Role::surface_container_high]);
            c.fill_rounded_rect({200.0, 200.0, 80.0, 40.0}, render::uniform_radii(20.0), 0xc0ff0000u);
            c.fill_shadow(render::rounded_rect_path({250.0, 300.0, 60.0, 60.0}, render::uniform_radii(12.0)), {0.0, 4.0, 6.0, 0x66000000u});
            c.fill_rounded_rect({250.0, 300.0, 60.0, 60.0}, render::uniform_radii(12.0), scheme()[color::Role::primary_container]);
        },
        &sink);
    expect_identical(r);
}

TEST_CASE("damage tracking repaints everything after a resize and stays exact while the size keeps changing") {
    const Report r = run(
        10, [](int f) { return f < 3 ? Size{300, 200} : f < 6 ? Size{320 + f, 180} : Size{240, 260}; }, opaque(), [](int) {},
        [&](render::Canvas& c, int f) {
            const double w = c.width();
            const double h = c.height();
            c.fill_rounded_rect({w - 80.0, h - 60.0, 64.0, 44.0}, render::uniform_radii(12.0), scheme()[color::Role::primary]);
            c.fill_rect({10.0 + f, 10.0, 50.0, 30.0}, scheme()[color::Role::secondary]);
        });
    expect_identical(r);
}

TEST_CASE("damage tracking stays exact for a theme switch, a translucent background, shadows at the edge and nested path clips") {
    const text::TextLine line = text::layout_line(fonts(), "Hamburgefonstiv 0123", {16.0, 24.0, 400, 0.0, 0.0});
    auto theme = [](int f) -> const color::ColorScheme& { return f >= 10 && f < 16 ? dark_scheme() : scheme(); };
    const Report r = run(
        26, fixed(640, 480), [&](int f) { return f >= 18 && f < 22 ? 0x80ffffffu : theme(f)[color::Role::surface]; }, [](int) {},
        [&](render::Canvas& c, int f) {
            const color::ColorScheme& s = theme(f);
            c.fill_shadow(render::rounded_rect_path({300.0 + (f % 4), 20.0, 40.0, 40.0}, render::uniform_radii(8.0)), {0.0, 6.0, 8.0, 0x80000000u});
            c.fill_shadow(render::rounded_rect_path({-20.0, 200.0, 60.0, 60.0}, render::uniform_radii(8.0)), {2.0, 3.0, 5.0, 0x80000000u});
            c.push_clip(render::rounded_rect_path({20.0, 20.0, 200.0, 160.0}, render::uniform_radii(24.0)));
            c.push_clip(render::circle_path({120.0 + f * 0.37, 100.0}, 70.0));
            c.fill_rect({0.0, 0.0, 320.0, 240.0}, s[color::Role::tertiary_container]);
            text::draw_line(c, fonts(), line, {30.0 + f * 0.25, 90.0}, s[color::Role::on_tertiary_container]);
            c.pop_clip();
            c.pop_clip();
            c.save();
            c.translate(160.0, 200.0);
            c.rotate(f * 0.05);
            c.fill_rect({-30.0, -10.0, 60.0, 20.0}, s[color::Role::primary]);
            c.restore();
            const color::Argb tint = f % 7 == 3 ? 0xff00ff00u + static_cast<color::Argb>(f) : 0xff00ff00u;
            c.draw_image(solid(4, 4, tint), {250.0, 150.0, 30.0, 30.0});
        });
    expect_identical(r);
    CHECK(r.repainted < r.full_area);
}

TEST_CASE("damage tracking stays exact under a translucent scrim in a canvas of odd width, as a moving pointer beside a switch") {
    const Report r = run(
        40, fixed(1081, 400), opaque(), [](int) {},
        [&](render::Canvas& c, int f) {
            c.save();
            c.scale(2.625);
            c.fill_path(render::ring_path({4.3, 30.2, 52.0, 32.0}, 16.0, 2.0), scheme()[color::Role::outline]);
            c.fill_path(render::circle_path({20.4, 46.2}, 12.0), scheme()[color::Role::on_surface_variant]);
            c.fill_rect({0.0, 0.0, 412.0, 160.0}, 0x52000000u);
            render::Path arrow;
            const render::Point p{50.0 + (f % 8) * 0.37, 50.0 + (f % 5) * 0.53};
            arrow.move_to(p);
            arrow.line_to({p.x, p.y + 12.0});
            arrow.line_to({p.x + 3.4, p.y + 8.6});
            arrow.line_to({p.x + 8.6, p.y + 8.6});
            arrow.close();
            c.fill_path(arrow, 0xff1c1b1fu);
            c.restore();
        });
    expect_identical(r);
    CHECK(r.repainted < r.full_area);
}
TEST_CASE("unchanged frames repaint nothing, and turning damage tracking off repaints the whole canvas") {
    thorvg::Canvas canvas(200, 100);
    lamina::diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    auto frame = [&](double x) {
        probe.begin_frame(0.0);
        canvas.clear(0xffffffffu);
        canvas.fill_rect({x, 10.0, 20.0, 20.0}, 0xffff0000u);
        canvas.finish();
        const lamina::diagnostics::FrameCounts counts = probe.counts();
        probe.end_frame();
        return counts;
    };
    CHECK(frame(10.0).repaint_pixels == 200 * 100);
    CHECK(canvas.take_changed_pixels().size() == 1);
    CHECK(frame(10.0).repaint_pixels == 0);
    CHECK(canvas.take_changed_pixels().empty());
    const lamina::diagnostics::FrameCounts moved = frame(12.0);
    CHECK(moved.repaint_pixels > 0);
    CHECK(moved.repaint_pixels < 200 * 100);
    CHECK(moved.repaint_tiles > 0);
    canvas.take_changed_pixels();
    canvas.set_damage_tracking(false);
    CHECK_FALSE(canvas.damage_tracking());
    CHECK(frame(12.0).repaint_pixels == 200 * 100);
    CHECK(frame(12.0).repaint_pixels == 200 * 100);
    canvas.set_damage_tracking(true);
    CHECK(frame(12.0).repaint_pixels == 200 * 100);
    CHECK(frame(12.0).repaint_pixels == 0);
    canvas.invalidate();
    CHECK(frame(12.0).repaint_pixels == 200 * 100);
    canvas.set_probe(nullptr);
}

TEST_CASE("paint added after a finish in the same frame is repainted and reported") {
    thorvg::Canvas canvas(100, 200);
    canvas.clear(0xffffffffu);
    canvas.finish();
    canvas.take_changed_pixels();
    canvas.clear(0xffffffffu);
    canvas.fill_rect({10.0, 10.0, 10.0, 10.0}, 0xffff0000u);
    CHECK(canvas.pixel(15, 15) == 0xffff0000u);
    canvas.fill_rect({70.0, 70.0, 10.0, 10.0}, 0xff0000ffu);
    CHECK(canvas.pixel(75, 75) == 0xff0000ffu);
    CHECK(canvas.pixel(15, 15) == 0xffff0000u);
    const std::vector<PixelRect> changed = canvas.take_changed_pixels();
    CHECK(inside(changed, 15, 15));
    CHECK(inside(changed, 75, 75));
    CHECK_FALSE(inside(changed, 50, 150));
}
TEST_CASE("layers above a video keep their pixels between frames and repaint only what changed in them") {
    AcceptAll sink;
    const render::Image photo = render::retained(gradient(64, 36));
    const int frames = 24;
    const Report r = run(
        frames, fixed(480, 270), [](int) { return 0xff000000u; }, [](int) {},
        [&](render::Canvas& c, int f) {
            c.draw_image(photo, {0.0, 0.0, 480.0, 270.0});
            c.fill_rect({0.0, 0.0, 480.0, 40.0}, 0xb3000000u);
            c.fill_rect({0.0, 200.0, 480.0, 70.0}, 0xb3000000u);
            c.fill_rounded_rect({210.0, 105.0, 60.0, 60.0}, render::uniform_radii(30.0), 0xff445566u);
            const double thumb = 40.0 + std::floor(f / 3) * 2.0;
            c.fill_rect({40.0, 218.0, 400.0, 4.0}, 0xff808080u);
            c.fill_rounded_rect({thumb, 212.0, 4.0, 16.0}, render::uniform_radii(2.0), 0xffffffffu);
        },
        &sink);
    expect_identical(r);
    CAPTURE(r.upper_uploaded);
    CAPTURE(r.upper_area);
    CHECK(r.upper_area == static_cast<std::int64_t>(480) * 270 * frames);
    CHECK(r.upper_uploaded < r.upper_area / 6);
}

TEST_CASE("an unchanged layer above a retained image repaints nothing, a change repaints its band, and moved bounds repaint the layer") {
    AcceptAll sink;
    const render::Image photo = render::retained(gradient(32, 32));
    thorvg::Canvas canvas(200, 200);
    canvas.set_retained_images(&sink);
    lamina::diagnostics::FrameProbe probe;
    probe.set_switches({false, true});
    canvas.set_probe(&probe);
    auto frame = [&](double label, double top) {
        probe.begin_frame(0.0);
        canvas.clear(0xffffffffu);
        canvas.draw_image(photo, {0.0, 0.0, 200.0, 200.0});
        canvas.fill_rect({0.0, top, 200.0, 20.0}, 0x80000000u);
        canvas.fill_rect({0.0, 180.0, 200.0, 20.0}, 0x80000000u);
        canvas.fill_rect({label, 184.0, 10.0, 10.0}, 0xffffffffu);
        canvas.finish();
        const lamina::diagnostics::FrameCounts counts = probe.counts();
        const std::vector<render::UpperLayer> uppers = canvas.upper_layers();
        canvas.take_changed_pixels();
        probe.end_frame();
        return std::pair{counts, uppers};
    };
    const auto first = frame(20.0, 0.0);
    REQUIRE(first.second.size() == 1);
    CHECK(first.first.repaint_pixels == 200 * 200 + 200 * 200);
    const auto same = frame(20.0, 0.0);
    CHECK(same.first.repaint_pixels == 0);
    CHECK(same.second.front().changed.empty());
    const auto label = frame(60.0, 0.0);
    REQUIRE(label.second.front().changed.size() == 1);
    const PixelRect band = label.second.front().changed.front();
    CHECK(band.width == 200);
    CHECK(band.y >= 128);
    CHECK(band.y + band.height <= 200);
    CHECK(label.first.repaint_pixels == band.area());
    const auto moved = frame(60.0, 10.0);
    REQUIRE(moved.second.front().changed.size() == 1);
    CHECK(moved.second.front().changed.front() == PixelRect{0, 10, 200, 190});
    canvas.set_probe(nullptr);
    canvas.set_retained_images(nullptr);
}
