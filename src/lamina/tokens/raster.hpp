#pragma once

#include <cstddef>

namespace lamina::tokens {

struct RasterTokens {
    int shadow_quality;
    double shadow_margin_sigmas;
    int shadow_shape_steps;
    std::size_t shadow_cache_bytes;
    double cull_glyph_outset_em;
};

inline constexpr RasterTokens raster{
    .shadow_quality = 100,
    .shadow_margin_sigmas = 3.0,
    .shadow_shape_steps = 4,
    .shadow_cache_bytes = 16u * 1024u * 1024u,
    .cull_glyph_outset_em = 0.5,
};

}
