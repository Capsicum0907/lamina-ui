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
    void clear(color::Argb color) override;
    render::PixelView finish() override;

    void resize(int width, int height);

protected:
    void fill_device_path(const render::Path& path, color::Argb color, render::FillRule rule) override;
    void fill_device_shadow(const render::Path& path, const render::Shadow& shadow) override;
    void begin_device_clip(const render::Rect& rect) override;
    void end_device_clip() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
