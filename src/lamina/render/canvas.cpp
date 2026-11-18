#include "lamina/render/canvas.hpp"

namespace lamina::render {

void Canvas::fill_path(const Path& path, color::Argb color, FillRule rule) {
    fill_device_path(path.transformed(transform_), color, rule);
}

void Canvas::fill_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow) {
    const double s = transform_.a;
    Shadow device = shadow;
    device.offset_x *= s;
    device.offset_y *= s;
    device.blur_sigma *= s;
    fill_device_path_with_shadow(path.transformed(transform_), color, device);
}

void Canvas::fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color) {
    fill_path(rounded_rect_path(rect, radii), color);
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
