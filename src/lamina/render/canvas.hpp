#pragma once

#include <cstdint>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/image.hpp"
#include "lamina/render/path.hpp"

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

class Canvas {
public:
    virtual ~Canvas() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void resize(int width, int height) = 0;
    virtual void clear(color::Argb color) = 0;
    virtual PixelView finish() = 0;

    void fill_path(const Path& path, color::Argb color, FillRule rule = FillRule::non_zero);
    void fill_shadow(const Path& path, const Shadow& shadow);
    void fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color);
    void fill_rect(const Rect& rect, color::Argb color);
    void draw_image(const Image& image, const Rect& rect, double opacity = 1.0);

    void save();
    void restore();
    void translate(double dx, double dy);
    void scale(double factor);
    void push_clip(const Rect& rect);
    void push_clip(const Path& path);
    void pop_clip();
    const Affine& transform() const { return transform_; }

    color::Argb pixel(int x, int y);

protected:
    virtual void fill_device_path(const Path& path, color::Argb color, FillRule rule) = 0;
    virtual void fill_device_shadow(const Path& path, const Shadow& shadow) = 0;
    virtual void fill_device_rect(Point from, Point to, color::Argb color) = 0;
    virtual void draw_device_image(const Image& image, const Affine& to_device, double opacity) = 0;
    virtual void begin_device_clip(const Rect& rect) = 0;
    virtual void begin_device_clip(const Path& path) = 0;
    virtual void end_device_clip() = 0;

private:
    Affine transform_{};
    std::vector<Affine> stack_;
};

}
