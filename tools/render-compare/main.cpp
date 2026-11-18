#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numbers>
#include <string>
#include <vector>

#include <blend2d.h>

#include "lamina/color/scheme.hpp"
#include "lamina/render/blend2d_canvas.hpp"
#include "lamina/render/thorvg_canvas.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/shape.hpp"

using namespace lamina;
using render::Canvas;
using render::Path;
using render::Point;

namespace {

struct Settings {
    int width = 1600;
    int height = 1000;
    double scale = 1.25;
    int frames = 120;
    int zoom = 8;
    int crop = 48;
};

Path star_path(Point center, double outer, double inner, int points, double bulge) {
    Path p;
    const int n = points * 2;
    auto vertex = [&](int i) {
        const double r = (i % 2 == 0) ? outer : inner;
        const double t = std::numbers::pi * 2.0 * i / n - std::numbers::pi / 2.0;
        return Point{center.x + r * std::cos(t), center.y + r * std::sin(t)};
    };
    auto tangent = [&](int i, double len) {
        const double t = std::numbers::pi * 2.0 * i / n - std::numbers::pi / 2.0;
        return Point{-std::sin(t) * len, std::cos(t) * len};
    };
    const double handle = bulge * outer * std::numbers::pi / n;
    p.move_to(vertex(0));
    for (int i = 0; i < n; ++i) {
        const Point a = vertex(i);
        const Point b = vertex(i + 1);
        const Point ta = tangent(i, handle);
        const Point tb = tangent(i + 1, handle);
        p.cubic_to({a.x + ta.x, a.y + ta.y}, {b.x - tb.x, b.y - tb.y}, b);
    }
    p.close();
    return p;
}

void draw_scene(Canvas& canvas, const color::ColorScheme& scheme, const Settings& s, bool shadows) {
    using color::Role;
    using tokens::CornerSize;
    canvas.clear(scheme[Role::surface]);
    canvas.save();
    canvas.scale(s.scale);

    const double w = s.width / s.scale;
    const CornerSize sizes[] = {CornerSize::extra_small, CornerSize::small,       CornerSize::medium,
                                CornerSize::large,       CornerSize::extra_large, CornerSize::full};
    const Role fills[] = {Role::surface_container_low, Role::primary_container, Role::secondary_container,
                          Role::tertiary_container, Role::surface_container_highest};

    int index = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 8; ++col, ++index) {
            const render::Rect r{24.0 + col * (w - 48.0) / 8.0 + 0.3 * row, 24.0 + row * 96.0 + 0.37 * col,
                                 (w - 48.0) / 8.0 - 16.0, 72.0};
            const CornerSize size = sizes[index % std::size(sizes)];
            const double radius = tokens::corner_radius(size, r.width, r.height);
            const auto fill = scheme[fills[index % std::size(fills)]];
            const auto path = render::rounded_rect_path(r, render::uniform_radii(radius));
            if (shadows && index % 3 == 0) {
                const double elevation = tokens::elevation_dp[1 + index % 5];
                canvas.fill_path_with_shadow(path, fill, {0.0, elevation, elevation, 0x4d000000});
            } else {
                canvas.fill_path(path, fill);
            }
        }
    }

    for (int i = 0; i < 24; ++i) {
        const double cx = 60.0 + i * (w - 120.0) / 23.0;
        const double cy = 470.0 + 40.0 * std::sin(i * 0.7);
        const double r = 36.0 + 10.0 * std::cos(i * 1.3);
        const auto c = (scheme[i % 2 == 0 ? Role::primary : Role::tertiary] & 0x00ffffff) | 0x80000000;
        canvas.fill_rounded_rect({cx - r, cy - r, 2 * r, 2 * r}, render::uniform_radii(r), c);
    }

    for (int i = 0; i < 12; ++i) {
        const double cx = 70.0 + i * (w - 140.0) / 11.0;
        const int points = 4 + i % 9;
        const double inner = 0.55 + 0.03 * (i % 7);
        canvas.fill_path(star_path({cx, 620.0}, 44.0, 44.0 * inner, points, 0.6 + 0.05 * (i % 5)),
                         scheme[i % 2 == 0 ? Role::secondary : Role::primary]);
    }

    for (int i = 0; i < 40; ++i) {
        const double y = 700.0 + i * 1.37;
        canvas.fill_rounded_rect({24.0, y, w - 48.0, 0.8}, render::uniform_radii(0), scheme[Role::outline]);
    }

    canvas.restore();
}

double time_ms(Canvas& canvas, const color::ColorScheme& scheme, const Settings& s, bool shadows) {
    draw_scene(canvas, scheme, s, shadows);
    canvas.finish();
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < s.frames; ++i) {
        draw_scene(canvas, scheme, s, shadows);
        canvas.finish();
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count() / s.frames;
}

BLImage to_image(const render::PixelView& v) {
    BLImage img(v.width, v.height, BL_FORMAT_PRGB32);
    BLImageData d{};
    img.makeMutable(&d);
    for (int y = 0; y < v.height; ++y) {
        std::memcpy(static_cast<std::uint8_t*>(d.pixelData) + d.stride * y, v.data + v.stride * y,
                    static_cast<std::size_t>(v.width) * 4);
    }
    return img;
}

BLImage zoomed_crop(const render::PixelView& v, int x0, int y0, int size, int zoom) {
    BLImage img(size * zoom, size * zoom, BL_FORMAT_PRGB32);
    BLImageData d{};
    img.makeMutable(&d);
    for (int y = 0; y < size * zoom; ++y) {
        auto* dst = reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(d.pixelData) + d.stride * y);
        const auto* src = reinterpret_cast<const std::uint32_t*>(v.data + v.stride * (y0 + y / zoom));
        for (int x = 0; x < size * zoom; ++x) {
            dst[x] = src[x0 + x / zoom];
        }
    }
    return img;
}

struct Diff {
    double mean;
    int max;
    double over8_percent;
};

Diff compare(const render::PixelView& a, const render::PixelView& b) {
    double sum = 0.0;
    int max = 0;
    long long over = 0;
    for (int y = 0; y < a.height; ++y) {
        const auto* ra = a.data + a.stride * y;
        const auto* rb = b.data + b.stride * y;
        for (int x = 0; x < a.width; ++x) {
            int worst = 0;
            for (int c = 0; c < 4; ++c) {
                const int d = std::abs(int(ra[x * 4 + c]) - int(rb[x * 4 + c]));
                sum += d;
                worst = std::max(worst, d);
            }
            max = std::max(max, worst);
            if (worst > 8) {
                ++over;
            }
        }
    }
    const double pixels = double(a.width) * a.height;
    return {sum / (pixels * 4), max, 100.0 * over / pixels};
}

}

int main(int argc, char** argv) {
    const std::filesystem::path out = argc > 1 ? argv[1] : "render-compare-out";
    std::filesystem::create_directories(out);
    Settings s;
    const auto scheme = color::spec_2021().build(color::default_scheme_options);

    render::Blend2DCanvas b2d(s.width, s.height);
    render::ThorVGCanvas tvg(s.width, s.height);

    const double b2d_flat = time_ms(b2d, scheme, s, false);
    const double tvg_flat = time_ms(tvg, scheme, s, false);
    const double b2d_shadow = time_ms(b2d, scheme, s, true);
    const double tvg_shadow = time_ms(tvg, scheme, s, true);

    draw_scene(b2d, scheme, s, true);
    draw_scene(tvg, scheme, s, true);
    const auto vb = b2d.finish();
    const auto vt = tvg.finish();
    to_image(vb).writeToFile((out / "blend2d.png").string().c_str());
    to_image(vt).writeToFile((out / "thorvg.png").string().c_str());

    struct Crop {
        const char* name;
        int x;
        int y;
    };
    const Crop crops[] = {{"corner", 24, 24}, {"shadow", 24, 100}, {"star", 60, 730}, {"hairlines", 600, 868}};
    for (const auto& c : crops) {
        zoomed_crop(vb, c.x, c.y, s.crop, s.zoom).writeToFile((out / (std::string("crop-") + c.name + "-blend2d.png")).string().c_str());
        zoomed_crop(vt, c.x, c.y, s.crop, s.zoom).writeToFile((out / (std::string("crop-") + c.name + "-thorvg.png")).string().c_str());
    }

    const Diff d = compare(vb, vt);
    std::printf("canvas %dx%d, %d frames each\n", s.width, s.height, s.frames);
    std::printf("no shadows: blend2d %.2f ms/frame, thorvg %.2f ms/frame\n", b2d_flat, tvg_flat);
    std::printf("shadows:    blend2d %.2f ms/frame, thorvg %.2f ms/frame\n", b2d_shadow, tvg_shadow);
    std::printf("pixel diff: mean %.3f, max %d, pixels over 8: %.3f%%\n", d.mean, d.max, d.over8_percent);
    return 0;
}
