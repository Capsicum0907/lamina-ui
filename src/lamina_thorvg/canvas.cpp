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
    struct ClipLevel {
        tvg::Shape* clipper = nullptr;
        bool clipper_used = false;
        std::vector<tvg::Scene*> scenes;
    };

    struct Upper {
        tvg::Scene* root = nullptr;
        bool attached = false;
        tvg::SwCanvas* canvas = nullptr;
        std::vector<std::uint32_t> buffer;
        int width = 0;
        bool listed = false;
    };

    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> buffer;
    tvg::SwCanvas* canvas = nullptr;
    tvg::Scene* base = nullptr;
    bool base_attached = false;
    bool opaque = false;
    bool dirty = false;
    std::vector<ClipLevel> clips;
    std::vector<Upper> uppers;
    std::vector<render::UpperLayer> drawn;
    std::size_t layer = 0;
    ShadowCache shadows;
    tvg::SwCanvas* shadow_canvas = nullptr;
    std::uint64_t frame = 0;

    Impl(int w, int h) {
        tvg::Initializer::init(worker_threads());
        canvas = tvg::SwCanvas::gen(tvg::EngineOption::None);
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
        drop_uppers();
        drop_base();
        delete canvas;
        for (Upper& u : uppers) {
            delete u.canvas;
        }
        delete shadow_canvas;
        tvg::Initializer::term();
    }

    Upper& upper(std::size_t l) {
        if (uppers.size() < l) {
            uppers.resize(l);
        }
        Upper& u = uppers[l - 1];
        if (!u.canvas) {
            u.canvas = tvg::SwCanvas::gen();
        }
        if (!u.root) {
            u.root = tvg::Scene::gen();
        }
        return u;
    }

    void drop_base() {
        canvas->sync();
        canvas->remove();
        if (base && !base_attached) {
            tvg::Paint::rel(base);
        }
        base = nullptr;
        base_attached = false;
    }

    void zero(const render::PixelRect& r) {
        for (int y = r.y; y < r.y + r.height; ++y) {
            std::uint32_t* row = buffer.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + r.x;
            std::fill(row, row + r.width, 0u);
        }
    }

    void add_to_root(std::size_t l, tvg::Paint* paint) {
        if (l == 0) {
            if (!base) {
                base = tvg::Scene::gen();
            }
            base->add(paint);
        } else {
            upper(l).root->add(paint);
        }
    }

    tvg::Scene* clip_scene(std::size_t depth, std::size_t l) {
        ClipLevel& c = clips[depth];
        if (c.scenes.size() <= l) {
            c.scenes.resize(l + 1, nullptr);
        }
        if (!c.scenes[l]) {
            auto* scene = tvg::Scene::gen();
            auto* clipper = c.clipper_used ? static_cast<tvg::Shape*>(c.clipper->duplicate()) : c.clipper;
            c.clipper_used = true;
            scene->clip(clipper);
            if (depth == 0) {
                add_to_root(l, scene);
            } else {
                clip_scene(depth - 1, l)->add(scene);
            }
            c.scenes[l] = scene;
        }
        return c.scenes[l];
    }

    void add(tvg::Paint* paint) {
        if (clips.empty()) {
            add_to_root(layer, paint);
        } else {
            clip_scene(clips.size() - 1, layer)->add(paint);
        }
        dirty = true;
    }

    void pop_clip() {
        ClipLevel& c = clips.back();
        if (!c.clipper_used) {
            tvg::Paint::rel(c.clipper);
        }
        clips.pop_back();
    }

    void drop_clips() {
        while (!clips.empty()) {
            pop_clip();
        }
    }

    void drop_uppers() {
        for (Upper& u : uppers) {
            if (u.attached) {
                u.canvas->sync();
                u.canvas->remove();
            } else if (u.root) {
                tvg::Paint::rel(u.root);
            }
            u.root = nullptr;
            u.attached = false;
            u.listed = false;
        }
        drawn.clear();
    }

    void fit_upper(Upper& u, const render::PixelRect& b) {
        const std::size_t size = static_cast<std::size_t>(b.width) * static_cast<std::size_t>(b.height);
        if (u.buffer.size() != size || u.width != b.width) {
            u.buffer.assign(size, 0u);
            u.width = b.width;
        }
    }

    void attach_upper(Upper& u, const render::PixelRect& b) {
        u.canvas->target(u.buffer.data(), static_cast<std::uint32_t>(b.width), static_cast<std::uint32_t>(b.width), static_cast<std::uint32_t>(b.height),
                         tvg::ColorSpace::ARGB8888);
        u.root->translate(static_cast<float>(-b.x), static_cast<float>(-b.y));
        u.canvas->add(u.root);
        u.attached = true;
    }

    void zero_upper(Upper& u, const render::PixelRect& r) {
        for (int y = r.y; y < r.y + r.height; ++y) {
            std::uint32_t* row = u.buffer.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(u.width) + r.x;
            std::fill(row, row + r.width, 0u);
        }
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

void Canvas::clear_device(color::Argb color) {
    impl_->drop_clips();
    impl_->drop_base();
    impl_->drop_uppers();
    impl_->opaque = (color >> 24) == 0xffu;
    impl_->layer = 0;
    impl_->frame += 1;
    impl_->shadows.trim(rt.shadow_cache_bytes, impl_->frame);
    auto* bg = tvg::Shape::gen();
    bg->appendRect(0.0f, 0.0f, static_cast<float>(impl_->width), static_cast<float>(impl_->height));
    const Rgba c = split(color);
    bg->fill(c.r, c.g, c.b, c.a);
    impl_->add(bg);
}

void Canvas::select_device_layer(std::size_t layer) {
    impl_->layer = layer;
}

PixelView Canvas::finish() {
    if (impl_->dirty) {
        std::vector<std::pair<std::size_t, render::PixelRect>> bounds;
        for (std::size_t l = 1; l < layer_count() && l <= impl_->uppers.size(); ++l) {
            if (const std::optional<render::PixelRect> b = upper_pixel_bounds(l); b && impl_->uppers[l - 1].root && !impl_->uppers[l - 1].listed) {
                bounds.emplace_back(l, *b);
            }
        }
        const std::vector<render::PixelRect> rects = repaint_rects();
        tvg::SwCanvas* base = impl_->canvas;
        const bool painting = impl_->base && !rects.empty();
        if (!impl_->opaque || !impl_->base) {
            for (const render::PixelRect& r : rects) {
                impl_->zero(r);
            }
        }
        auto pass = [&](const render::PixelRect& r) {
            base->sync();
            base->viewport(r.x, r.y, r.width, r.height);
            if (!impl_->base_attached) {
                base->add(impl_->base);
                impl_->base_attached = true;
            }
            base->update();
        };
        for (std::size_t k = 0; painting && k < rects.size(); ++k) {
            {
                const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_update);
                pass(rects[k]);
            }
            const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_draw);
            base->draw(false);
            base->sync();
        }
        std::size_t rasterized = 0;
        for (const auto& [l, b] : bounds) {
            const UpperRepaint repaint = upper_repaint(l, b);
            Impl::Upper& u = impl_->uppers[l - 1];
            impl_->fit_upper(u, b);
            u.listed = true;
            impl_->drawn.push_back({{reinterpret_cast<const std::uint8_t*>(u.buffer.data()), b.width, b.height, static_cast<std::intptr_t>(b.width) * 4}, b.x,
                                    b.y, l, repaint.changed});
            if (repaint.rects.empty()) {
                continue;
            }
            impl_->attach_upper(u, b);
            rasterized += 1;
            for (const render::PixelRect& r : repaint.rects) {
                impl_->zero_upper(u, r);
                {
                    const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_update);
                    u.canvas->sync();
                    u.canvas->viewport(r.x, r.y, r.width, r.height);
                    u.canvas->update();
                }
                const diagnostics::StageScope stage(probe(), diagnostics::Stage::raster_draw);
                u.canvas->draw(false);
                u.canvas->sync();
            }
        }
        impl_->dirty = false;
        if (probe() && probe()->counting()) {
            probe()->counts().layers = 1 + rasterized;
        }
    }
    return {reinterpret_cast<const std::uint8_t*>(impl_->buffer.data()), impl_->width, impl_->height,
            static_cast<std::intptr_t>(impl_->width) * 4};
}

std::vector<render::UpperLayer> Canvas::upper_layers() const {
    return impl_->drawn;
}

void Canvas::resize(int width, int height) {
    if (width != impl_->width || height != impl_->height) {
        impl_->canvas->sync();
        impl_->retarget(width, height);
        invalidate();
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
    impl_->clips.push_back({clipper, false, {}});
}

void Canvas::begin_device_clip(const Path& path) {
    impl_->clips.push_back({to_shape(path, 0xff000000), false, {}});
}

void Canvas::end_device_clip() {
    if (!impl_->clips.empty()) {
        impl_->pop_clip();
    }
}

}
