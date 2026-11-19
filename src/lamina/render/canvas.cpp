#include "lamina/render/canvas.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::render {

void Canvas::fill_path(const Path& path, color::Argb color, FillRule rule) {
    fill_device_path(path.transformed(transform_), color, rule);
}

void Canvas::fill_shadow(const Path& path, const Shadow& shadow) {
    const double s = transform_.a;
    Shadow device = shadow;
    device.offset_x *= s;
    device.offset_y *= s;
    device.blur_sigma *= s;
    fill_device_shadow(path.transformed(transform_), device);
}

void Canvas::fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color) {
    fill_path(rounded_rect_path(rect, radii), color);
}

void Canvas::fill_rect(const Rect& rect, color::Argb color) {
    fill_device_rect(transform_.apply({rect.x, rect.y}), transform_.apply({rect.x + rect.width, rect.y + rect.height}), color);
}

void Canvas::draw_image(const Image& image, const Rect& rect, double opacity) {
    if (image.width <= 0 || image.height <= 0 || image.pixels.size() < static_cast<std::size_t>(image.width) * image.height || opacity <= 0.0) {
        return;
    }
    const Affine place{rect.width / image.width, 0.0, 0.0, rect.height / image.height, rect.x, rect.y};
    draw_device_image(image, transform_.then_local(place), std::min(opacity, 1.0));
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

void Canvas::push_clip(const Rect& rect) {
    const Point a = transform_.apply({rect.x, rect.y});
    const Point b = transform_.apply({rect.x + rect.width, rect.y + rect.height});
    begin_device_clip({std::min(a.x, b.x), std::min(a.y, b.y), std::abs(b.x - a.x), std::abs(b.y - a.y)});
}

void Canvas::push_clip(const Path& path) {
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
