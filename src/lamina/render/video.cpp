#include "lamina/render/video.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

#include "lamina/tokens/video.hpp"

namespace lamina::render {

namespace {

int half(int n) {
    return (n + 1) / 2;
}

std::uint32_t channel(double v) {
    return static_cast<std::uint32_t>(std::clamp(static_cast<int>(std::lround(v)), 0, tokens::video.channel_max));
}

}

bool VideoFrame::valid() const {
    if (width <= 0 || height <= 0 || y_pitch < width) {
        return false;
    }
    const auto rows = static_cast<std::size_t>(height);
    const auto chroma_rows = static_cast<std::size_t>(half(height));
    if (y.size() < static_cast<std::size_t>(y_pitch) * rows) {
        return false;
    }
    if (layout == VideoLayout::nv12) {
        return uv_pitch >= 2 * half(width) && u.size() >= static_cast<std::size_t>(uv_pitch) * chroma_rows;
    }
    return uv_pitch >= half(width) && u.size() >= static_cast<std::size_t>(uv_pitch) * chroma_rows &&
           v.size() >= static_cast<std::size_t>(uv_pitch) * chroma_rows;
}

std::shared_ptr<const ImageIdentity> video_surface() {
    static std::atomic<std::uint64_t> next{1};
    return std::make_shared<const ImageIdentity>(ImageIdentity{next.fetch_add(1)});
}

Image video_image(const VideoFrame& f) {
    Image out;
    if (!f.valid()) {
        return out;
    }
    const tokens::VideoTokens& t = tokens::video;
    const tokens::YuvToRgb& k = f.matrix == VideoMatrix::bt601 ? t.bt601 : t.bt709;
    const double y0 = f.full_range ? 0.0 : t.limited_y_offset;
    const double ys = f.full_range ? 1.0 : t.limited_y_scale;
    const double cs = f.full_range ? 1.0 : t.limited_c_scale;
    out.width = f.width;
    out.height = f.height;
    out.pixels.resize(static_cast<std::size_t>(f.width) * static_cast<std::size_t>(f.height));
    for (int row = 0; row < f.height; ++row) {
        const std::size_t crow = static_cast<std::size_t>(row / 2) * static_cast<std::size_t>(f.uv_pitch);
        for (int col = 0; col < f.width; ++col) {
            const auto c = static_cast<std::size_t>(col / 2);
            const double cb = (f.layout == VideoLayout::nv12 ? f.u[crow + 2 * c] : f.u[crow + c]) - t.chroma_mid;
            const double cr = (f.layout == VideoLayout::nv12 ? f.u[crow + 2 * c + 1] : f.v[crow + c]) - t.chroma_mid;
            const double luma = (f.y[static_cast<std::size_t>(row) * static_cast<std::size_t>(f.y_pitch) + static_cast<std::size_t>(col)] - y0) * ys;
            const double r = luma + k.cr_to_r * cr * cs;
            const double g = luma - (k.cb_to_g * cb + k.cr_to_g * cr) * cs;
            const double b = luma + k.cb_to_b * cb * cs;
            out.pixels[static_cast<std::size_t>(row) * static_cast<std::size_t>(f.width) + static_cast<std::size_t>(col)] =
                t.opaque | (channel(r) << 16) | (channel(g) << 8) | channel(b);
        }
    }
    return out;
}

}
