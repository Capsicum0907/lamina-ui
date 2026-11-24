#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/render/damage.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/image.hpp"
#include "lamina/render/layer_plan.hpp"
#include "lamina/render/path.hpp"
#include "lamina/render/video.hpp"
#include "lamina/tokens/raster.hpp"

namespace lamina::diagnostics {
class FrameProbe;
}

namespace lamina::render {

struct PixelView {
    const std::uint8_t* data;
    int width;
    int height;
    std::intptr_t stride;
};

enum class FillRule { non_zero, even_odd };

struct Shadow {
    double offset_x;
    double offset_y;
    double blur_sigma;
    color::Argb color;
};

struct RetainedDraw {
    const Image* image;
    Affine to_device;
    double opacity;
    Rect clip;
    const VideoFrame* video = nullptr;
};

struct UpperLayer {
    PixelView pixels;
    int x;
    int y;
    std::size_t layer;
    std::vector<PixelRect> changed;
};

class RetainedImageSink {
public:
    virtual ~RetainedImageSink() = default;
    virtual bool accepts(const Image& image) const = 0;
    virtual bool accepts_video(const VideoFrame&) const { return false; }
};

class Canvas {
public:
    virtual ~Canvas() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void resize(int width, int height) = 0;
    void clear(color::Argb color);
    virtual PixelView finish() = 0;
    virtual std::vector<UpperLayer> upper_layers() const { return {}; }

    void set_retained_images(const RetainedImageSink* sink) { sink_ = sink; }
    const RetainedImageSink* retained_images() const { return sink_; }
    const std::vector<std::vector<RetainedDraw>>& retained_draws() const { return batches_; }
    void prefetch_image(const Image& image);
    const std::vector<const Image*>& prefetched_images() const { return prefetched_; }

    void fill_path(const Path& path, color::Argb color, FillRule rule = FillRule::non_zero);
    void fill_shadow(const Path& path, const Shadow& shadow);
    void fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color);
    void fill_rect(const Rect& rect, color::Argb color);
    void draw_image(const Image& image, const Rect& rect, double opacity = 1.0);
    void draw_video(const VideoFrame& frame, const Rect& rect, double opacity = 1.0);

    void save();
    void restore();
    void translate(double dx, double dy);
    void scale(double factor);
    void rotate(double radians);
    void push_clip(const Rect& rect);
    void push_clip(const Path& path);
    void pop_clip();
    const Affine& transform() const { return transform_; }
    void allow_minified_images();
    bool minified_images_allowed() const { return minified_; }

    bool quick_reject(const Rect& local, double outset = 0.0) const;
    Rect visible_bounds() const;
    void count_rejected(std::size_t points);

    color::Argb pixel(int x, int y);

    void set_probe(diagnostics::FrameProbe* probe) { probe_ = probe; }
    diagnostics::FrameProbe* probe() const { return probe_; }

    void set_damage_tracking(bool on);
    bool damage_tracking() const { return damage_on_; }
    void invalidate();
    std::vector<PixelRect> take_changed_pixels();

protected:
    struct UpperRepaint {
        bool full = true;
        std::vector<PixelRect> rects;
        std::vector<PixelRect> changed;
    };

    virtual bool tracks_damage() const { return false; }
    virtual bool repaints_whole_rows() const { return false; }
    std::vector<PixelRect> repaint_rects();
    std::optional<PixelRect> upper_pixel_bounds(std::size_t layer) const;
    UpperRepaint upper_repaint(std::size_t layer, const PixelRect& bounds);
    virtual void clear_device(color::Argb color) = 0;
    virtual void select_device_layer(std::size_t) {}
    std::optional<Rect> layer_bounds(std::size_t layer) const { return plan_.bounds(layer); }
    std::size_t layer_count() const { return plan_.layers(); }
    virtual void fill_device_path(const Path& path, color::Argb color, FillRule rule) = 0;
    virtual void fill_device_shadow(const Path& path, const Shadow& shadow) = 0;
    virtual void fill_device_rect(Point from, Point to, color::Argb color) = 0;
    virtual void draw_device_image(const Image& image, const Affine& to_device, double opacity) = 0;
    virtual void begin_device_clip(const Rect& rect) = 0;
    virtual void begin_device_clip(const Path& path) = 0;
    virtual void end_device_clip() = 0;

private:
    double covered(std::span<const Point> device_points);
    void push_clip_bounds(const Rect& device);
    void place_paint(const Rect& device);
    bool fingerprinting() const;
    void note(const Rect& device, Fingerprint print);
    std::uint64_t clip_hash() const { return clip_hashes_.empty() ? 0 : clip_hashes_.back(); }
    bool retain(const Image& image, const Affine& m, double opacity, std::span<const Point> corners);
    bool retain_video(const VideoFrame& frame, const Affine& m, double opacity, std::span<const Point> corners);
    const Image& cpu_video(const VideoFrame& frame);

    struct UpperDamage {
        DamageGrid grid;
        PixelRect bounds{0, 0, 0, 0};
        bool drawn = false;
        std::vector<PixelRect> changed;
        std::vector<std::pair<Rect, std::uint64_t>> notes;
    };

    void add_changed(std::vector<PixelRect>& changed, const PixelRect& r) const;

    struct CpuVideo {
        const ImageIdentity* surface;
        std::uint64_t generation;
        Image image;
        bool used;
    };

    struct Saved {
        Affine transform;
        bool minified;
    };

    Affine transform_{};
    bool minified_ = false;
    diagnostics::FrameProbe* probe_ = nullptr;
    std::vector<Saved> stack_;
    std::vector<Rect> clip_bounds_;
    std::vector<bool> clip_is_path_;
    std::size_t path_clips_ = 0;
    const RetainedImageSink* sink_ = nullptr;
    LayerPlan plan_{tokens::raster.layers_max};
    std::vector<std::vector<RetainedDraw>> batches_;
    std::vector<const Image*> prefetched_;
    std::size_t layer_ = 0;
    bool damage_on_ = tokens::raster.damage_tracking;
    DamageGrid damage_;
    std::vector<std::uint64_t> clip_hashes_;
    std::vector<PixelRect> changed_;
    std::vector<UpperDamage> uppers_;
    std::vector<std::unique_ptr<CpuVideo>> cpu_videos_;
};

}
