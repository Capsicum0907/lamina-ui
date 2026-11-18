#include "lamina/render/blend2d_canvas.hpp"

#include <algorithm>
#include <cmath>

#include <blend2d.h>

#include "lamina/render/box_blur.hpp"

namespace lamina::render {

namespace {

constexpr double shadow_extent_sigmas = 3.0;

BLPath to_bl_path(const Path& path) {
    BLPath out;
    const auto& pts = path.points();
    std::size_t i = 0;
    for (const PathVerb verb : path.verbs()) {
        switch (verb) {
        case PathVerb::move:
            out.moveTo(pts[i].x, pts[i].y);
            i += 1;
            break;
        case PathVerb::line:
            out.lineTo(pts[i].x, pts[i].y);
            i += 1;
            break;
        case PathVerb::cubic:
            out.cubicTo(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, pts[i + 2].x, pts[i + 2].y);
            i += 3;
            break;
        case PathVerb::close:
            out.close();
            break;
        }
    }
    return out;
}

}

struct Blend2DCanvas::Impl {
    BLImage image;
    BLContext context;
    bool painting = false;

    BLContext& ctx() {
        if (!painting) {
            context.begin(image);
            painting = true;
        }
        return context;
    }

    void end() {
        if (painting) {
            context.end();
            painting = false;
        }
    }
};

Blend2DCanvas::Blend2DCanvas(int width, int height) : impl_(std::make_unique<Impl>()) {
    impl_->image.create(width, height, BL_FORMAT_PRGB32);
}

Blend2DCanvas::~Blend2DCanvas() {
    impl_->end();
}

int Blend2DCanvas::width() const {
    return impl_->image.width();
}

int Blend2DCanvas::height() const {
    return impl_->image.height();
}

void Blend2DCanvas::clear(color::Argb color) {
    BLContext& c = impl_->ctx();
    c.setCompOp(BL_COMP_OP_SRC_COPY);
    c.fillAll(BLRgba32(color));
    c.setCompOp(BL_COMP_OP_SRC_OVER);
}

PixelView Blend2DCanvas::finish() {
    impl_->end();
    BLImageData data{};
    impl_->image.getData(&data);
    return {static_cast<const std::uint8_t*>(data.pixelData), data.size.w, data.size.h, data.stride};
}

void Blend2DCanvas::fill_device_path(const Path& path, color::Argb color) {
    impl_->ctx().fillPath(to_bl_path(path), BLRgba32(color));
}

void Blend2DCanvas::fill_device_path_with_shadow(const Path& path, color::Argb color, const Shadow& shadow) {
    const BLPath bl = to_bl_path(path);
    BLBox box{};
    bl.getBoundingBox(&box);
    const double margin = std::ceil(shadow.blur_sigma * shadow_extent_sigmas) + 1.0;
    const int x0 = static_cast<int>(std::floor(box.x0 + shadow.offset_x - margin));
    const int y0 = static_cast<int>(std::floor(box.y0 + shadow.offset_y - margin));
    const int x1 = static_cast<int>(std::ceil(box.x1 + shadow.offset_x + margin));
    const int y1 = static_cast<int>(std::ceil(box.y1 + shadow.offset_y + margin));
    const int w = std::max(1, x1 - x0);
    const int h = std::max(1, y1 - y0);

    BLImage layer(w, h, BL_FORMAT_PRGB32);
    {
        BLContext lc(layer);
        lc.clearAll();
        lc.translate(shadow.offset_x - x0, shadow.offset_y - y0);
        lc.fillPath(bl, BLRgba32(shadow.color));
        lc.end();
    }
    BLImageData data{};
    layer.makeMutable(&data);
    gaussian_blur_prgb32(static_cast<std::uint8_t*>(data.pixelData), w, h, data.stride, shadow.blur_sigma);

    BLContext& c = impl_->ctx();
    c.blitImage(BLPointI(x0, y0), layer);
    c.fillPath(bl, BLRgba32(color));
}

}
