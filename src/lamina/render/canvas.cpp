#include "lamina/render/canvas.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/tokens/raster.hpp"
#include "lamina/tokens/video.hpp"

namespace lamina::render {

namespace {

Rect bounds_of(std::span<const Point> pts) {
    if (pts.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
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
    return {x0, y0, x1 - x0, y1 - y0};
}

Rect device_rect(const Affine& m, const Rect& r) {
    const std::array<Point, 4> corners{m.apply({r.x, r.y}), m.apply({r.x + r.width, r.y}), m.apply({r.x + r.width, r.y + r.height}),
                                       m.apply({r.x, r.y + r.height})};
    return bounds_of(corners);
}

Rect intersect(const Rect& a, const Rect& b) {
    const double x0 = std::max(a.x, b.x);
    const double y0 = std::max(a.y, b.y);
    const double x1 = std::min(a.x + a.width, b.x + b.width);
    const double y1 = std::min(a.y + a.height, b.y + b.height);
    return {x0, y0, std::max(0.0, x1 - x0), std::max(0.0, y1 - y0)};
}

Rect grow(const Rect& r, double by) {
    return {r.x - by, r.y - by, r.width + 2.0 * by, r.height + 2.0 * by};
}

enum class Command : std::uint64_t { background = 1, path, shadow, rect, image, rect_clip, path_clip };

Fingerprint print_of(Command command) {
    Fingerprint f;
    f.add(static_cast<std::uint64_t>(command));
    return f;
}

void add_path(Fingerprint& f, const Path& path) {
    f.add(static_cast<std::uint64_t>(path.verbs().size()));
    for (const PathVerb v : path.verbs()) {
        f.add(static_cast<std::uint64_t>(v));
    }
    for (const Point& p : path.points()) {
        f.add(p);
    }
}

DamageSettings damage_settings() {
    const tokens::RasterTokens& t = tokens::raster;
    return {t.damage_tile, t.damage_rects_max, t.damage_merge_input_max, t.damage_full_fraction};
}

std::size_t tiles_across(int pixels) {
    const int t = std::max(1, tokens::raster.damage_tile);
    return pixels > 0 ? static_cast<std::size_t>((pixels + t - 1) / t) : 0;
}

}

void Canvas::clear(color::Argb color) {
    std::erase_if(cpu_videos_, [](const std::unique_ptr<CpuVideo>& v) { return !v->used; });
    for (const auto& v : cpu_videos_) {
        v->used = false;
    }
    plan_.reset();
    batches_.clear();
    prefetched_.clear();
    clip_bounds_.clear();
    clip_is_path_.clear();
    path_clips_ = 0;
    layer_ = 0;
    clip_hashes_.clear();
    if (tracks_damage() && damage_on_) {
        damage_.begin(width(), height(), tokens::raster.damage_tile);
        Fingerprint f = print_of(Command::background);
        f.add(static_cast<std::uint64_t>(color));
        note({0.0, 0.0, static_cast<double>(width()), static_cast<double>(height())}, f);
    } else {
        damage_.invalidate();
    }
    clear_device(color);
}

void Canvas::set_damage_tracking(bool on) {
    if (on != damage_on_) {
        damage_on_ = on;
        damage_.invalidate();
    }
}

void Canvas::invalidate() {
    damage_.invalidate();
}

bool Canvas::fingerprinting() const {
    return layer_ == 0 && damage_on_ && damage_.begun();
}

void Canvas::note(const Rect& device, Fingerprint print) {
    if (!fingerprinting()) {
        return;
    }
    const double o = tokens::raster.damage_outset;
    const Rect all{0.0, 0.0, static_cast<double>(width()), static_cast<double>(height())};
    const Rect area = intersect(intersect(grow(device, o), grow(visible_bounds(), o)), all);
    if (area.width <= 0.0 || area.height <= 0.0) {
        return;
    }
    print.add(clip_hash());
    damage_.add(area, print.value());
}

std::vector<PixelRect> Canvas::repaint_rects() {
    DamageGrid::Result result;
    if (tracks_damage() && damage_on_) {
        DamageSettings settings = damage_settings();
        settings.whole_rows = repaints_whole_rows();
        result = damage_.commit(settings);
    } else {
        damage_.invalidate();
        result.full = true;
        result.tiles = tiles_across(width()) * tiles_across(height());
    }
    if (result.full) {
        result.rects.clear();
        if (width() > 0 && height() > 0) {
            result.rects.push_back({0, 0, width(), height()});
        }
    }
    std::uint64_t pixels = 0;
    for (const PixelRect& r : result.rects) {
        pixels += static_cast<std::uint64_t>(r.area());
        changed_.push_back(r);
    }
    if (changed_.size() > tokens::raster.damage_pending_max) {
        PixelRect all{0, 0, 0, 0};
        for (const PixelRect& r : changed_) {
            all = pixel_union(all, r);
        }
        changed_.assign(1, all);
    }
    if (probe_ && probe_->counting()) {
        probe_->counts().repaint_tiles += result.tiles;
        probe_->counts().repaint_pixels += pixels;
    }
    return result.rects;
}

std::vector<PixelRect> Canvas::take_changed_pixels() {
    return std::exchange(changed_, {});
}

void Canvas::place_paint(const Rect& device) {
    if (sink_) {
        layer_ = plan_.place_paint(intersect(device, visible_bounds()));
        select_device_layer(layer_);
    }
}

void Canvas::prefetch_image(const Image& image) {
    if (sink_ && image.identity && image.width > 0 && image.height > 0 &&
        image.pixels.size() >= static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) && sink_->accepts(image)) {
        prefetched_.push_back(&image);
    }
}

bool Canvas::retain(const Image& image, const Affine& m, double opacity, std::span<const Point> corners) {
    if (!sink_ || !image.identity || path_clips_ > 0) {
        return false;
    }
    const double min_scale = minified_ ? 0.0 : tokens::raster.retained_min_scale;
    if (std::hypot(m.a, m.b) < min_scale || std::hypot(m.c, m.d) < min_scale || !sink_->accepts(image)) {
        return false;
    }
    const Rect visible = visible_bounds();
    const std::optional<std::size_t> batch = plan_.place_image(intersect(bounds_of(corners), visible));
    if (!batch) {
        return false;
    }
    if (batches_.size() <= *batch) {
        batches_.resize(*batch + 1);
    }
    batches_[*batch].push_back({&image, m, opacity, visible});
    return true;
}

bool Canvas::retain_video(const VideoFrame& frame, const Affine& m, double opacity, std::span<const Point> corners) {
    if (!sink_ || !frame.surface || path_clips_ > 0) {
        return false;
    }
    const double min_scale = minified_ ? 0.0 : tokens::raster.retained_min_scale;
    if (std::hypot(m.a, m.b) < min_scale || std::hypot(m.c, m.d) < min_scale || !sink_->accepts_video(frame)) {
        return false;
    }
    const Rect visible = visible_bounds();
    const std::optional<std::size_t> batch = plan_.place_image(intersect(bounds_of(corners), visible));
    if (!batch) {
        return false;
    }
    if (batches_.size() <= *batch) {
        batches_.resize(*batch + 1);
    }
    batches_[*batch].push_back({nullptr, m, opacity, visible, &frame});
    return true;
}

const Image& Canvas::cpu_video(const VideoFrame& frame) {
    const ImageIdentity* key = frame.surface.get();
    for (const auto& v : cpu_videos_) {
        if (key && v->surface == key && v->generation == frame.generation) {
            v->used = true;
            return v->image;
        }
    }
    std::erase_if(cpu_videos_, [key](const std::unique_ptr<CpuVideo>& v) { return key && v->surface == key && !v->used; });
    if (cpu_videos_.size() >= tokens::video.cpu_frames_kept) {
        std::erase_if(cpu_videos_, [](const std::unique_ptr<CpuVideo>& v) { return !v->used; });
    }
    cpu_videos_.push_back(std::make_unique<CpuVideo>(CpuVideo{key, frame.generation, retained(video_image(frame)), true}));
    return cpu_videos_.back()->image;
}

void Canvas::draw_video(const VideoFrame& frame, const Rect& rect, double opacity) {
    if (!frame.valid() || opacity <= 0.0) {
        return;
    }
    if (quick_reject(rect)) {
        if (probe_ && probe_->counting()) {
            probe_->counts().images += 1;
        }
        return;
    }
    const Affine place{rect.width / frame.width, 0.0, 0.0, rect.height / frame.height, rect.x, rect.y};
    const Affine m = transform_.then_local(place);
    const auto w = static_cast<double>(frame.width);
    const auto h = static_cast<double>(frame.height);
    const std::array<Point, 4> corners{m.apply({0.0, 0.0}), m.apply({w, 0.0}), m.apply({w, h}), m.apply({0.0, h})};
    if (retain_video(frame, m, std::min(opacity, 1.0), corners)) {
        if (probe_ && probe_->counting()) {
            diagnostics::FrameCounts& c = probe_->counts();
            c.images += 1;
            c.gpu_images += 1;
            c.image_pixels += static_cast<std::uint64_t>(frame.width) * static_cast<std::uint64_t>(frame.height);
            c.covered_pixels += covered(corners);
        }
        return;
    }
    draw_image(cpu_video(frame), rect, opacity);
}

Rect Canvas::visible_bounds() const {
    const Rect all{0.0, 0.0, static_cast<double>(width()), static_cast<double>(height())};
    return clip_bounds_.empty() ? all : intersect(all, clip_bounds_.back());
}

bool Canvas::quick_reject(const Rect& local, double outset) const {
    const Rect grown{local.x - outset, local.y - outset, local.width + 2.0 * outset, local.height + 2.0 * outset};
    const Rect device = device_rect(transform_, grown);
    const Rect visible = visible_bounds();
    return device.x >= visible.x + visible.width || device.y >= visible.y + visible.height || device.x + device.width <= visible.x ||
           device.y + device.height <= visible.y || visible.width <= 0.0 || visible.height <= 0.0;
}

void Canvas::count_rejected(std::size_t points) {
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        c.paths += 1;
        c.offscreen_paths += 1;
        c.path_points += points;
    }
}

void Canvas::push_clip_bounds(const Rect& device) {
    clip_bounds_.push_back(clip_bounds_.empty() ? device : intersect(clip_bounds_.back(), device));
}

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
    if (quick_reject(bounds_of(path.points()))) {
        count_rejected(path.points().size());
        return;
    }
    Path device = path.transformed(transform_);
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        const double area = covered(device.points());
        c.paths += 1;
        c.offscreen_paths += area > 0.0 ? 0 : 1;
        c.path_points += device.points().size();
        c.covered_pixels += area;
    }
    const Rect bounds = bounds_of(device.points());
    if (sink_) {
        place_paint(bounds);
    }
    if (fingerprinting()) {
        Fingerprint f = print_of(Command::path);
        f.add(static_cast<std::uint64_t>(color)).add(static_cast<std::uint64_t>(rule));
        add_path(f, device);
        note(bounds, f);
    }
    fill_device_path(device, color, rule);
}

void Canvas::fill_shadow(const Path& path, const Shadow& shadow) {
    const double reach = std::max(std::abs(shadow.offset_x), std::abs(shadow.offset_y)) + shadow.blur_sigma * tokens::raster.shadow_margin_sigmas;
    if (quick_reject(bounds_of(path.points()), reach)) {
        if (probe_ && probe_->counting()) {
            probe_->counts().shadows += 1;
            probe_->counts().path_points += path.points().size();
        }
        return;
    }
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
    const double grown = std::max(std::abs(device.offset_x), std::abs(device.offset_y)) + device.blur_sigma * tokens::raster.shadow_margin_sigmas;
    const Rect spread = grow(bounds_of(outline.points()), grown);
    if (sink_) {
        place_paint(spread);
    }
    if (fingerprinting()) {
        Fingerprint f = print_of(Command::shadow);
        f.add(device.offset_x).add(device.offset_y).add(device.blur_sigma).add(static_cast<std::uint64_t>(device.color));
        add_path(f, outline);
        note(grow(spread, 1.0), f);
    }
    fill_device_shadow(outline, device);
}

void Canvas::fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color) {
    fill_path(rounded_rect_path(rect, radii), color);
}

void Canvas::fill_rect(const Rect& rect, color::Argb color) {
    if (quick_reject(rect)) {
        count_rejected(2);
        return;
    }
    const Point from = transform_.apply({rect.x, rect.y});
    const Point to = transform_.apply({rect.x + rect.width, rect.y + rect.height});
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        const std::array<Point, 2> corners{from, to};
        c.paths += 1;
        c.path_points += corners.size();
        c.covered_pixels += covered(corners);
    }
    const std::array<Point, 2> corners{from, to};
    if (sink_) {
        place_paint(bounds_of(corners));
    }
    if (fingerprinting()) {
        Fingerprint f = print_of(Command::rect);
        f.add(from).add(to).add(static_cast<std::uint64_t>(color));
        note(bounds_of(corners), f);
    }
    fill_device_rect(from, to, color);
}

void Canvas::draw_image(const Image& image, const Rect& rect, double opacity) {
    if (image.width <= 0 || image.height <= 0 || image.pixels.size() < static_cast<std::size_t>(image.width) * image.height || opacity <= 0.0) {
        return;
    }
    if (quick_reject(rect)) {
        if (probe_ && probe_->counting()) {
            probe_->counts().images += 1;
        }
        return;
    }
    const Affine place{rect.width / image.width, 0.0, 0.0, rect.height / image.height, rect.x, rect.y};
    const Affine m = transform_.then_local(place);
    const auto w = static_cast<double>(image.width);
    const auto h = static_cast<double>(image.height);
    const std::array<Point, 4> corners{m.apply({0.0, 0.0}), m.apply({w, 0.0}), m.apply({w, h}), m.apply({0.0, h})};
    const bool kept = retain(image, m, std::min(opacity, 1.0), corners);
    if (probe_ && probe_->counting()) {
        diagnostics::FrameCounts& c = probe_->counts();
        c.images += 1;
        c.gpu_images += kept ? 1 : 0;
        c.image_pixels += static_cast<std::uint64_t>(image.width) * static_cast<std::uint64_t>(image.height);
        c.covered_pixels += covered(corners);
    }
    if (kept) {
        return;
    }
    if (sink_) {
        place_paint(bounds_of(corners));
    }
    if (fingerprinting()) {
        Fingerprint f = print_of(Command::image);
        f.add(m.a).add(m.b).add(m.c).add(m.d).add(m.tx).add(m.ty).add(std::min(opacity, 1.0));
        f.add(static_cast<std::uint64_t>(image.width)).add(static_cast<std::uint64_t>(image.height));
        if (image.identity) {
            f.add(image.identity->serial);
        } else {
            f.add_bytes({reinterpret_cast<const std::uint8_t*>(image.pixels.data()), image.pixels.size() * sizeof(color::Argb)});
        }
        note(bounds_of(corners), f);
    }
    draw_device_image(image, m, std::min(opacity, 1.0));
}

void Canvas::save() {
    stack_.push_back({transform_, minified_});
}

void Canvas::restore() {
    if (!stack_.empty()) {
        transform_ = stack_.back().transform;
        minified_ = stack_.back().minified;
        stack_.pop_back();
    }
}

void Canvas::allow_minified_images() {
    minified_ = true;
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
    const Rect device{std::min(a.x, b.x), std::min(a.y, b.y), std::abs(b.x - a.x), std::abs(b.y - a.y)};
    push_clip_bounds(device);
    clip_is_path_.push_back(false);
    Fingerprint f = print_of(Command::rect_clip);
    f.add(device.x).add(device.y).add(device.width).add(device.height);
    clip_hashes_.push_back(Fingerprint::combine(clip_hash(), f.value()));
    begin_device_clip(device);
}

void Canvas::push_clip(const Path& path) {
    if (probe_ && probe_->counting()) {
        probe_->counts().clips += 1;
    }
    const Path device = path.transformed(transform_);
    push_clip_bounds(bounds_of(device.points()));
    clip_is_path_.push_back(true);
    path_clips_ += 1;
    Fingerprint f = print_of(Command::path_clip);
    add_path(f, device);
    clip_hashes_.push_back(Fingerprint::combine(clip_hash(), f.value()));
    begin_device_clip(device);
}

void Canvas::pop_clip() {
    if (!clip_bounds_.empty()) {
        clip_bounds_.pop_back();
    }
    if (!clip_is_path_.empty()) {
        path_clips_ -= clip_is_path_.back() ? 1 : 0;
        clip_is_path_.pop_back();
    }
    if (!clip_hashes_.empty()) {
        clip_hashes_.pop_back();
    }
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
