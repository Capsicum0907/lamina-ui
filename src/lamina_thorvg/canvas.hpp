#pragma once

#include <memory>

#include "lamina/render/canvas.hpp"

namespace lamina::thorvg {

class Canvas final : public render::Canvas {
public:
    Canvas(int width, int height);
    ~Canvas() override;

    int width() const override;
    int height() const override;
    void resize(int width, int height) override;
    void clear(color::Argb color) override;
    render::PixelView finish() override;

protected:
    void fill_device_path(const render::Path& path, color::Argb color, render::FillRule rule) override;
    void fill_device_shadow(const render::Path& path, const render::Shadow& shadow) override;
    void fill_device_rect(render::Point from, render::Point to, color::Argb color) override;
    void draw_device_image(const render::Image& image, const render::Affine& to_device, double opacity) override;
    void begin_device_clip(const render::Rect& rect) override;
    void end_device_clip() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
