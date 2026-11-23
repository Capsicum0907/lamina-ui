#include "lamina/render/canvas.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "lamina/diagnostics/frame_stats.hpp"

namespace lamina::render {

double Canvas::covered(std::span<const Point> pts) {
    if (pts.empty()) {
        return 0.0;
    }
    double x0 = pts.front().x;
    double y0 = pts.front().y;
    double x1 = x0;
    double y1 = y0;
    for (const Point& p : pts) {
        x0 = std::min(x0, p.x);
        y0 = std::min(y0, p.y);
        x1 = std::max(x1, p.x);
        y1 = std::max(y1, p.y);
    }
    const double w = std::clamp(x1, 0.0, static_cast<double>(width())) - std::clamp(x0, 0.0, static_cast<double>(width()));
    const double h = std::clamp(y1, 0.0, static_cast<double>(height())) - std::clamp(y0, 0.0, static_cast<double>(height()));
    return std::max(0.0, w) * std::max(0.0, h);
}

void Canvas::fill_path(const Path& path, color::Argb color, FillRule rule) {
    Path device = path.transformed(transform_);
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        const double area = covered(device.points());
        c.paths += 1;
        c.offscreen_paths += area > 0.0 ? 0 : 1;
        c.path_points += device.points().size();
        c.covered_pixels += area;
    }
    fill_device_path(device, color, rule);
}

void Canvas::fill_shadow(const Path& path, const Shadow& shadow) {
    const double s = std::hypot(transform_.a, transform_.b);
    Shadow device = shadow;
    device.offset_x *= s;
    device.offset_y *= s;
    device.blur_sigma *= s;
    const Path outline = path.transformed(transform_);
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        c.shadows += 1;
        c.path_points += outline.points().size();
        c.covered_pixels += covered(outline.points());
    }
    fill_device_shadow(outline, device);
}

void Canvas::fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color) {
    fill_path(rounded_rect_path(rect, radii), color);
}

void Canvas::fill_rect(const Rect& rect, color::Argb color) {
    const Point from = transform_.apply({rect.x, rect.y});
    const Point to = transform_.apply({rect.x + rect.width, rect.y + rect.height});
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        const std::array<Point, 2> corners{from, to};
        c.paths += 1;
        c.path_points += corners.size();
        c.covered_pixels += covered(corners);
    }
    fill_device_rect(from, to, color);
}

void Canvas::draw_image(const Image& image, const Rect& rect, double opacity) {
    if (image.width <= 0 || image.height <= 0 || image.pixels.size() < static_cast<std::size_t>(image.width) * image.height || opacity <= 0.0) {
        return;
    }
    const Affine place{rect.width / image.width, 0.0, 0.0, rect.height / image.height, rect.x, rect.y};
    const Affine m = transform_.then_local(place);
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        const auto w = static_cast<double>(image.width);
        const auto h = static_cast<double>(image.height);
        const std::array<Point, 4> corners{m.apply({0.0, 0.0}), m.apply({w, 0.0}), m.apply({w, h}), m.apply({0.0, h})};
        c.images += 1;
        c.image_pixels += static_cast<std::uint64_t>(image.width) * static_cast<std::uint64_t>(image.height);
        c.covered_pixels += covered(corners);
    }
    draw_device_image(image, m, std::min(opacity, 1.0));
}

void Canvas::save() {
    stack_.push_back(transform_);
}

void Canvas::restore() {
    if (!stack_.empty()) {
        transform_ = stack_.back();
        stack_.pop_back();
    }
}

void Canvas::translate(double dx, double dy) {
    transform_ = transform_.then_local({1.0, 0.0, 0.0, 1.0, dx, dy});
}

void Canvas::scale(double factor) {
    transform_ = transform_.then_local({factor, 0.0, 0.0, factor, 0.0, 0.0});
}

void Canvas::rotate(double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    transform_ = transform_.then_local({c, s, -s, c, 0.0, 0.0});
}

void Canvas::push_clip(const Rect& rect) {
    if (probe_ && probe_->counting()) {
        probe_->counts().clips += 1;
    }
    const Point a = transform_.apply({rect.x, rect.y});
    const Point b = transform_.apply({rect.x + rect.width, rect.y + rect.height});
    begin_device_clip({std::min(a.x, b.x), std::min(a.y, b.y), std::abs(b.x - a.x), std::abs(b.y - a.y)});
}

void Canvas::push_clip(const Path& path) {
    if (probe_ && probe_->counting()) {
        probe_->counts().clips += 1;
    }
    begin_device_clip(path.transformed(transform_));
}

void Canvas::pop_clip() {
    end_device_clip();
}

color::Argb Canvas::pixel(int x, int y) {
    const PixelView view = finish();
    const auto* row = reinterpret_cast<const std::uint32_t*>(view.data + view.stride * y);
    const std::uint32_t prgb = row[x];
    const std::uint32_t a = prgb >> 24;
    if (a == 0) {
        return 0;
    }
    auto unpremultiply = [a](std::uint32_t c) { return (c * 255 + a / 2) / a; };
    const std::uint32_t r = unpremultiply((prgb >> 16) & 0xff);
    const std::uint32_t g = unpremultiply((prgb >> 8) & 0xff);
    const std::uint32_t b = unpremultiply(prgb & 0xff);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

}
