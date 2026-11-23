#include "lamina_thorvg/canvas.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <thread>
#include <unordered_map>
#include <vector>

#include <thorvg.h>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/tokens/raster.hpp"

namespace lamina::thorvg {

using render::FillRule;
using render::Path;
using render::PathVerb;
using render::PixelView;
using render::Rect;
using render::Shadow;

namespace {

constexpr const tokens::RasterTokens& rt = tokens::raster;

std::uint32_t worker_threads() {
    const unsigned cores = std::thread::hardware_concurrency();
    const unsigned spare = cores > rt.reserved_cores ? cores - rt.reserved_cores : 0u;
    return static_cast<std::uint32_t>(std::min(spare, rt.worker_threads_max));
}

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

struct ShadowKey {
    std::vector<std::int32_t> coords;
    std::vector<PathVerb> verbs;
    double sigma = 0.0;
    color::Argb color = 0;

    bool operator==(const ShadowKey&) const = default;

    std::size_t hash() const {
        std::size_t h = std::hash<double>{}(sigma) ^ (std::hash<std::uint32_t>{}(color) << 1);
        for (const std::int32_t c : coords) {
            h = h * hash_prime ^ static_cast<std::size_t>(static_cast<std::uint32_t>(c));
        }
        for (const PathVerb v : verbs) {
            h = h * hash_prime ^ static_cast<std::size_t>(v);
        }
        return h;
    }

    static constexpr std::size_t hash_prime = 1099511628211ull;
};

struct ShadowImage {
    ShadowKey key;
    std::size_t hash = 0;
    int width = 0;
    int height = 0;
    int margin = 0;
    std::vector<std::uint32_t> pixels;
    std::uint64_t used = 0;

    std::size_t bytes() const { return pixels.size() * sizeof(std::uint32_t); }
};

class ShadowCache {
public:
    ShadowImage* find(const ShadowKey& key, std::size_t hash, std::uint64_t frame) {
        auto [first, last] = index_.equal_range(hash);
        for (auto it = first; it != last; ++it) {
            if (it->second->key == key) {
                entries_.splice(entries_.begin(), entries_, it->second);
                it->second->used = frame;
                return &*it->second;
            }
        }
        return nullptr;
    }

    ShadowImage& insert(ShadowImage image) {
        bytes_ += image.bytes();
        entries_.push_front(std::move(image));
        index_.emplace(entries_.front().hash, entries_.begin());
        return entries_.front();
    }

    void trim(std::size_t budget, std::uint64_t frame) {
        while (bytes_ > budget && !entries_.empty() && entries_.back().used != frame) {
            auto [first, last] = index_.equal_range(entries_.back().hash);
            for (auto it = first; it != last; ++it) {
                if (&*it->second == &entries_.back()) {
                    index_.erase(it);
                    break;
                }
            }
            bytes_ -= entries_.back().bytes();
            entries_.pop_back();
        }
    }

private:
    std::list<ShadowImage> entries_;
    std::unordered_multimap<std::size_t, std::list<ShadowImage>::iterator> index_;
    std::size_t bytes_ = 0;
};

Path quantized_path(const ShadowKey& key, double steps, double margin) {
    Path local;
    std::size_t i = 0;
    auto at = [&](std::size_t k) { return render::Point{key.coords[2 * k] / steps + margin, key.coords[2 * k + 1] / steps + margin}; };
    for (const PathVerb verb : key.verbs) {
        switch (verb) {
        case PathVerb::move:
            local.move_to(at(i));
            i += 1;
            break;
        case PathVerb::line:
            local.line_to(at(i));
            i += 1;
            break;
        case PathVerb::cubic:
            local.cubic_to(at(i), at(i + 1), at(i + 2));
            i += 3;
            break;
        case PathVerb::close:
            local.close();
            break;
        }
    }
    return local;
}

}

struct Canvas::Impl {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> buffer;
    tvg::SwCanvas* canvas = nullptr;
    bool dirty = false;
    std::vector<std::pair<tvg::Scene*, tvg::Shape*>> clips;
    ShadowCache shadows;
    tvg::SwCanvas* shadow_canvas = nullptr;
    std::uint64_t frame = 0;

    Impl(int w, int h) {
        tvg::Initializer::init(worker_threads());
        canvas = tvg::SwCanvas::gen();
        shadow_canvas = tvg::SwCanvas::gen();
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
        delete shadow_canvas;
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
        for (auto& [scene, clipper] : clips) {
            tvg::Paint::rel(scene);
            tvg::Paint::rel(clipper);
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
    impl_->canvas->sync();
    impl_->canvas->remove();
    impl_->frame += 1;
    impl_->shadows.trim(rt.shadow_cache_bytes, impl_->frame);
    auto* bg = tvg::Shape::gen();
    bg->appendRect(0.0f, 0.0f, static_cast<float>(impl_->width), static_cast<float>(impl_->height));
    const Rgba c = split(color);
    bg->fill(c.r, c.g, c.b, c.a);
    impl_->add(bg);
}

PixelView Canvas::finish() {
    if (impl_->dirty) {
        {
            const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_update);
            impl_->canvas->update();
        }
        const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_draw);
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

void Canvas::fill_device_rect(render::Point from, render::Point to, color::Argb color) {
    auto* shape = tvg::Shape::gen();
    const auto x0 = static_cast<float>(from.x);
    const auto y0 = static_cast<float>(from.y);
    const auto x1 = static_cast<float>(to.x);
    const auto y1 = static_cast<float>(to.y);
    shape->moveTo(x0, y0);
    shape->lineTo(x1, y0);
    shape->lineTo(x1, y1);
    shape->lineTo(x0, y1);
    shape->lineTo(x0, y0);
    shape->close();
    const Rgba c = split(color);
    shape->fill(c.r, c.g, c.b, c.a);
    impl_->add(shape);
}

void Canvas::draw_device_image(const render::Image& image, const render::Affine& m, double opacity) {
    auto* picture = tvg::Picture::gen();
    tvg::Result loaded = tvg::Result::Unknown;
    {
        const diagnostics::StageScope stage(probe(), diagnostics::Stage::image_copy);
        loaded = picture->load(image.pixels.data(), static_cast<std::uint32_t>(image.width), static_cast<std::uint32_t>(image.height),
                               tvg::ColorSpace::ARGB8888S, true);
    }
    if (loaded != tvg::Result::Success) {
        tvg::Paint::rel(picture);
        return;
    }
    if (probe() && probe()->counting()) {
        probe()->counts().image_bytes += static_cast<std::uint64_t>(image.width) * static_cast<std::uint64_t>(image.height) * sizeof(std::uint32_t);
    }
    auto f = [](double v) { return static_cast<float>(v); };
    picture->transform({f(m.a), f(m.c), f(m.tx), f(m.b), f(m.d), f(m.ty), 0.0f, 0.0f, 1.0f});
    picture->opacity(static_cast<std::uint8_t>(std::lround(opacity * 255.0)));
    impl_->add(picture);
}

void Canvas::fill_device_shadow(const Path& path, const Shadow& shadow) {
    if ((shadow.color >> 24) == 0) {
        return;
    }
    const Path moved = path.transformed({1.0, 0.0, 0.0, 1.0, shadow.offset_x, shadow.offset_y});
    if (shadow.blur_sigma <= 0.0) {
        fill_device_path(moved, shadow.color, FillRule::non_zero);
        return;
    }
    const auto& pts = moved.points();
    if (pts.empty()) {
        return;
    }
    double x0 = pts.front().x;
    double y0 = pts.front().y;
    for (const render::Point& p : pts) {
        x0 = std::min(x0, p.x);
        y0 = std::min(y0, p.y);
    }
    const double steps = rt.shadow_shape_steps;
    ShadowKey key;
    key.coords.reserve(pts.size() * 2);
    std::int32_t max_x = 0;
    std::int32_t max_y = 0;
    for (const render::Point& p : pts) {
        const auto qx = static_cast<std::int32_t>(std::lround((p.x - x0) * steps));
        const auto qy = static_cast<std::int32_t>(std::lround((p.y - y0) * steps));
        key.coords.push_back(qx);
        key.coords.push_back(qy);
        max_x = std::max(max_x, qx);
        max_y = std::max(max_y, qy);
    }
    key.verbs.assign(moved.verbs().begin(), moved.verbs().end());
    key.sigma = shadow.blur_sigma;
    key.color = shadow.color;
    const std::size_t hash = key.hash();
    ShadowImage* image = impl_->shadows.find(key, hash, impl_->frame);
    if (!image) {
        ShadowImage made;
        made.margin = static_cast<int>(std::ceil(shadow.blur_sigma * rt.shadow_margin_sigmas));
        made.width = static_cast<int>(std::ceil(max_x / steps)) + 2 * made.margin + 1;
        made.height = static_cast<int>(std::ceil(max_y / steps)) + 2 * made.margin + 1;
        made.pixels.assign(static_cast<std::size_t>(made.width) * static_cast<std::size_t>(made.height), 0);
        tvg::SwCanvas* off = impl_->shadow_canvas;
        off->target(made.pixels.data(), static_cast<std::uint32_t>(made.width), static_cast<std::uint32_t>(made.width),
                    static_cast<std::uint32_t>(made.height), tvg::ColorSpace::ARGB8888);
        auto* scene = tvg::Scene::gen();
        scene->add(to_shape(quantized_path(key, steps, made.margin), shadow.color));
        scene->add(tvg::SceneEffect::GaussianBlur, shadow.blur_sigma, 0, 0, rt.shadow_quality);
        off->add(scene);
        off->draw(true);
        off->sync();
        off->remove();
        made.key = std::move(key);
        made.hash = hash;
        made.used = impl_->frame;
        image = &impl_->shadows.insert(std::move(made));
        if (probe() && probe()->counting()) {
            probe()->counts().shadow_renders += 1;
        }
    }
    auto* picture = tvg::Picture::gen();
    if (picture->load(image->pixels.data(), static_cast<std::uint32_t>(image->width), static_cast<std::uint32_t>(image->height),
                      tvg::ColorSpace::ARGB8888, false) != tvg::Result::Success) {
        tvg::Paint::rel(picture);
        return;
    }
    picture->translate(static_cast<float>(std::round(x0) - image->margin), static_cast<float>(std::round(y0) - image->margin));
    impl_->add(picture);
}

void Canvas::begin_device_clip(const Rect& rect) {
    auto* clipper = tvg::Shape::gen();
    clipper->appendRect(static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width),
                        static_cast<float>(rect.height));
    impl_->clips.emplace_back(tvg::Scene::gen(), clipper);
}

void Canvas::begin_device_clip(const Path& path) {
    impl_->clips.emplace_back(tvg::Scene::gen(), to_shape(path, 0xff000000));
}

void Canvas::end_device_clip() {
    if (impl_->clips.empty()) {
        return;
    }
    auto [scene, clipper] = impl_->clips.back();
    impl_->clips.pop_back();
    scene->clip(clipper);
    impl_->add(scene);
}

}
