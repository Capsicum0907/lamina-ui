#include "lamina_thorvg/canvas.hpp"

#include <cmath>
#include <vector>

#include <thorvg.h>

namespace lamina::thorvg {

using render::FillRule;
using render::Path;
using render::PathVerb;
using render::PixelView;
using render::Rect;
using render::Shadow;

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

struct Canvas::Impl {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> buffer;
    tvg::SwCanvas* canvas = nullptr;
    bool dirty = false;
    std::vector<std::pair<tvg::Scene*, Rect>> clips;

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
        drop_clips();
        delete canvas;
        tvg::Initializer::term();
    }

    void add(tvg::Paint* paint) {
        if (clips.empty()) {
            canvas->add(paint);
        } else {
            clips.back().first->add(paint);
        }
        dirty = true;
    }

    void drop_clips() {
        for (auto& [scene, rect] : clips) {
            tvg::Paint::rel(scene);
        }
        clips.clear();
    }
};

Canvas::Canvas(int width, int height) : impl_(std::make_unique<Impl>(width, height)) {}

Canvas::~Canvas() = default;

int Canvas::width() const {
    return impl_->width;
}

int Canvas::height() const {
    return impl_->height;
}

void Canvas::clear(color::Argb color) {
    impl_->drop_clips();
    impl_->canvas->remove();
    auto* bg = tvg::Shape::gen();
    bg->appendRect(0.0f, 0.0f, static_cast<float>(impl_->width), static_cast<float>(impl_->height));
    const Rgba c = split(color);
    bg->fill(c.r, c.g, c.b, c.a);
    impl_->add(bg);
}

PixelView Canvas::finish() {
    if (impl_->dirty) {
        impl_->canvas->update();
        impl_->canvas->draw(true);
        impl_->canvas->sync();
        impl_->dirty = false;
    }
    return {reinterpret_cast<const std::uint8_t*>(impl_->buffer.data()), impl_->width, impl_->height,
            static_cast<std::intptr_t>(impl_->width) * 4};
}

void Canvas::resize(int width, int height) {
    if (width != impl_->width || height != impl_->height) {
        impl_->canvas->sync();
        impl_->retarget(width, height);
    }
}

void Canvas::fill_device_path(const Path& path, color::Argb color, FillRule rule) {
    auto* shape = to_shape(path, color);
    shape->fillRule(rule == FillRule::even_odd ? tvg::FillRule::EvenOdd : tvg::FillRule::NonZero);
    impl_->add(shape);
}

void Canvas::fill_device_shadow(const Path& path, const Shadow& shadow) {
    if ((shadow.color >> 24) == 0) {
        return;
    }
    if (shadow.blur_sigma <= 0.0) {
        fill_device_path(path.transformed({1.0, 0.0, 0.0, 1.0, shadow.offset_x, shadow.offset_y}), shadow.color, FillRule::non_zero);
        return;
    }
    auto* scene = tvg::Scene::gen();
    scene->add(to_shape(path.transformed({1.0, 0.0, 0.0, 1.0, shadow.offset_x, shadow.offset_y}), shadow.color));
    scene->add(tvg::SceneEffect::GaussianBlur, shadow.blur_sigma, 0, 0, shadow_quality);
    impl_->add(scene);
}

void Canvas::begin_device_clip(const Rect& rect) {
    impl_->clips.emplace_back(tvg::Scene::gen(), rect);
}

void Canvas::end_device_clip() {
    if (impl_->clips.empty()) {
        return;
    }
    auto [scene, rect] = impl_->clips.back();
    impl_->clips.pop_back();
    auto* clipper = tvg::Shape::gen();
    clipper->appendRect(static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width),
                        static_cast<float>(rect.height));
    scene->clip(clipper);
    impl_->add(scene);
}

}
