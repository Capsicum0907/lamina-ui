#pragma once

#include <cstddef>
#include <cstdint>

namespace lamina::tokens {

struct YuvToRgb {
    double cr_to_r;
    double cb_to_g;
    double cr_to_g;
    double cb_to_b;
};

struct VideoTokens {
    YuvToRgb bt601;
    YuvToRgb bt709;
    double limited_y_offset;
    double limited_y_scale;
    double limited_c_scale;
    double chroma_mid;
    int channel_max;
    std::uint32_t opaque;
    std::size_t cpu_frames_kept;
};

inline constexpr VideoTokens video{
    .bt601 = {1.402, 0.344136, 0.714136, 1.772},
    .bt709 = {1.5748, 0.187324, 0.468124, 1.8556},
    .limited_y_offset = 16.0,
    .limited_y_scale = 255.0 / 219.0,
    .limited_c_scale = 255.0 / 224.0,
    .chroma_mid = 128.0,
    .channel_max = 255,
    .opaque = 0xFF000000u,
    .cpu_frames_kept = 4,
};

}
