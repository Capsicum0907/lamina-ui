#include "lamina/render/thorvg_canvas.hpp"

#include <cmath>
#include <numbers>
#include <vector>

#include <thorvg.h>

namespace lamina::render {

namespace {

constexpr int shadow_quality = 100;

struct Rgba {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

Rgba split(color::Argb c) {
    return {
        static_cast<std::uint8_t>((c >> 16) & 0xff),
        static_cast<std::uint8_t>((c >> 8) & 0xff),
        static_cast<std::uint8_t>(c & 0xff),
        static_cast<std::uint8_t>(c >> 24),
    };
}

tvg::Shape* to_shape(const Path& path, color::Argb color) {
    auto* shape = tvg::Shape::gen();
    const auto& pts = path.points();
    std::size_t i = 0;
    auto f = [](double v) { return static_cast<float>(v); };
    for (const PathVerb verb : path.verbs()) {
        switch (verb) {
        case PathVerb::move:
            shape->moveTo(f(pts[i].x), f(pts[i].y));
            i += 1;
            break;
        case PathVerb::line:
            shape->lineTo(f(pts[i].x), f(pts[i].y));
            i += 1;
            break;
        case PathVerb::cubic:
            shape->cubicTo(f(pts[i].x), f(pts[i].y), f(pts[i + 1].x), f(pts[i + 1].y), f(pts[i + 2].x), f(pts[i + 2].y));
            i += 3;
            break;
        case PathVerb::close:
            shape->close();
            break;
        }
    }
    const Rgba c = split(color);
    shape->fill(c.r, c.g, c.b, c.a);
    return shape;
}

}

struct ThorVGCanvas::Impl {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> buffer;
    tvg::SwCanvas* canvas = nullptr;
    bool dirty = false;

    Impl(int w, int h) {
        tvg::Initializer::init(0);
        canvas = tvg::SwCanvas::gen();
        retarget(w, h);
    }

    void retarget(int w, int h) {
        width = w;
        height = h;
        buffer.assign(static_cast<std::size_t>(w) * h, 0);
        canvas->target(buffer.data(), static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(w),
                       static_cast<std::uint32_t>(h), tvg::ColorSpace::ARGB8888);
        dirty = true;
    }

    ~Impl() {
        delete canvas;
        tvg::Initializer::term();
    }

    void add(tvg::Paint* paint) {
        canvas->add(paint);
        dirty = true;
    }
};

ThorVGCanvas::ThorVGCanvas(int width, int height) : impl_(std::make_unique<Impl>(width, height)) {}

ThorVGCanvas::~ThorVGCanvas() = default;

int ThorVGCanvas::width() const {
    return impl_->width;
}

int ThorVGCanvas::height() const {
    return impl_->height;
}

void ThorVGCanvas::clear(color::Argb color) {
    impl_->canvas->remove();
    auto* bg = tvg::Shape::gen();
    bg->appendRect(0.0f, 0.0f, static_cast<float>(impl_->width), static_cast<float>(impl_->height));
    const Rgba c = split(color);
    bg->fill(c.r, c.g, c.b, c.a);
    impl_->add(bg);
}

PixelView ThorVGCanvas::finish() {
    if (impl_->dirty) {
        impl_->canvas->update();
        impl_->canvas->draw(true);
        impl_->canvas->sync();
        impl_->dirty = false;
    }
    return {reinterpret_cast<const std::uint8_t*>(impl_->buffer.data()), impl_->width, impl_->height,
            static_cast<std::intptr_t>(impl_->width) * 4};
}

void ThorVGCanvas::resize(int width, int height) {
    if (width != impl_->width || height != impl_->height) {
        impl_->canvas->sync();
        impl_->retarget(width, height);
    }
}

void ThorVGCanvas::fill_device_path(const Path& path, color::Argb color) {
    impl_->add(to_shape(path, color));
}

void ThorVGCanvas::fill_device_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow) {
    if (shadow.blur_sigma <= 0.0 || (shadow.color >> 24) == 0) {
        fill_device_path(path, color);
        return;
    }
    auto* scene = tvg::Scene::gen();
    scene->add(to_shape(path, color));
    const Rgba s = split(shadow.color);
    const double distance = std::hypot(shadow.offset_x, shadow.offset_y);
    const double angle = std::atan2(shadow.offset_x, -shadow.offset_y) * 180.0 / std::numbers::pi;
    scene->add(tvg::SceneEffect::DropShadow, static_cast<int>(s.r), static_cast<int>(s.g), static_cast<int>(s.b),
               static_cast<int>(s.a), angle, distance, shadow.blur_sigma, shadow_quality);
    impl_->add(scene);
}

}
