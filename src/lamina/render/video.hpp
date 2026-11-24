#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "lamina/render/image.hpp"

namespace lamina::render {

enum class VideoLayout { nv12, i420, argb };
enum class VideoMatrix { bt601, bt709 };

struct VideoFrame {
    int width = 0;
    int height = 0;
    VideoLayout layout = VideoLayout::nv12;
    std::vector<std::uint8_t> y;
    std::vector<std::uint8_t> u;
    std::vector<std::uint8_t> v;
    std::vector<std::uint32_t> argb;
    int y_pitch = 0;
    int uv_pitch = 0;
    VideoMatrix matrix = VideoMatrix::bt709;
    bool full_range = false;
    std::shared_ptr<const ImageIdentity> surface = nullptr;
    std::uint64_t generation = 0;

    bool valid() const;
    std::size_t bytes() const { return y.size() + u.size() + v.size() + argb.size() * sizeof(std::uint32_t); }
};

std::shared_ptr<const ImageIdentity> video_surface();
Image video_image(const VideoFrame& frame);

}
