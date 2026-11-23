#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/image.hpp"
#include "lamina/render/layer_plan.hpp"
#include "lamina/render/path.hpp"
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
};

struct UpperLayer {
    PixelView pixels;
    int x;
    int y;
    std::size_t layer;
};

class RetainedImageSink {
public:
    virtual ~RetainedImageSink() = default;
    virtual bool accepts(const Image& image) const = 0;
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

    void fill_path(const Path& path, color::Argb color, FillRule rule = FillRule::non_zero);
    void fill_shadow(const Path& path, const Shadow& shadow);
    void fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color);
    void fill_rect(const Rect& rect, color::Argb color);
    void draw_image(const Image& image, const Rect& rect, double opacity = 1.0);

    void save();
    void restore();
    void translate(double dx, double dy);
    void scale(double factor);
    void rotate(double radians);
    void push_clip(const Rect& rect);
    void push_clip(const Path& path);
    void pop_clip();
    const Affine& transform() const { return transform_; }

    bool quick_reject(const Rect& local, double outset = 0.0) const;
    Rect visible_bounds() const;
    void count_rejected(std::size_t points);

    color::Argb pixel(int x, int y);

    void set_probe(diagnostics::FrameProbe* probe) { probe_ = probe; }
    diagnostics::FrameProbe* probe() const { return probe_; }

protected:
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
    bool retain(const Image& image, const Affine& m, double opacity, std::span<const Point> corners);

    Affine transform_{};
    diagnostics::FrameProbe* probe_ = nullptr;
    std::vector<Affine> stack_;
    std::vector<Rect> clip_bounds_;
    std::vector<bool> clip_is_path_;
    std::size_t path_clips_ = 0;
    const RetainedImageSink* sink_ = nullptr;
    LayerPlan plan_{tokens::raster.layers_max};
    std::vector<std::vector<RetainedDraw>> batches_;
};

}
