#pragma once

#include <cstdint>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"

namespace lamina::render {

struct PixelView {
    const std::uint8_t* data;
    int width;
    int height;
    std::intptr_t stride;
};

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
    virtual void clear(color::Argb color) = 0;
    virtual PixelView finish() = 0;

    void fill_path(const Path& path, color::Argb color);
    void fill_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow);
    void fill_rounded_rect(const Rect& rect, const CornerRadii& radii, color::Argb color);

    void save();
    void restore();
    void translate(double dx, double dy);
    void scale(double factor);
    const Affine& transform() const { return transform_; }

    color::Argb pixel(int x, int y);

protected:
    virtual void fill_device_path(const Path& path, color::Argb color) = 0;
    virtual void fill_device_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow) = 0;

private:
    Affine transform_{};
    std::vector<Affine> stack_;
};

}
