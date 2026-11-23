#pragma once

#include <cstddef>

namespace lamina::tokens {

struct RasterTokens {
    int shadow_quality;
    double shadow_margin_sigmas;
    int shadow_shape_steps;
    std::size_t shadow_cache_bytes;
    double cull_glyph_outset_em;
    unsigned worker_threads_max;
    unsigned reserved_cores;
    std::size_t layers_max;
    double retained_min_scale;
    std::size_t image_texture_bytes;
    bool image_texture_smooth;
};

inline constexpr RasterTokens raster{
    .shadow_quality = 100,
    .shadow_margin_sigmas = 3.0,
    .shadow_shape_steps = 4,
    .shadow_cache_bytes = 16u * 1024u * 1024u,
    .cull_glyph_outset_em = 0.5,
    .worker_threads_max = 3,
    .reserved_cores = 1,
    .layers_max = 3,
    .retained_min_scale = 0.5,
    .image_texture_bytes = 256u * 1024u * 1024u,
    .image_texture_smooth = true,
};

}
