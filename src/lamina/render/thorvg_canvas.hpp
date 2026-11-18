#pragma once

#include <memory>

#include "lamina/render/canvas.hpp"

namespace lamina::render {

class ThorVGCanvas final : public Canvas {
public:
    ThorVGCanvas(int width, int height);
    ~ThorVGCanvas() override;

    int width() const override;
    int height() const override;
    void clear(color::Argb color) override;
    PixelView finish() override;

protected:
    void fill_device_path(const Path& path, color::Argb color) override;
    void fill_device_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
