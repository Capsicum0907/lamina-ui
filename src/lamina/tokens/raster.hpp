#pragma once

#include <cstddef>
#include <string_view>

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
    bool damage_tracking;
    int damage_tile;
    std::size_t damage_rects_max;
    std::size_t damage_merge_input_max;
    double damage_full_fraction;
    double damage_outset;
    std::size_t damage_pending_max;
    std::string_view full_repaint_environment;
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
    .damage_tracking = true,
    .damage_tile = 64,
    .damage_rects_max = 1,
    .damage_merge_input_max = 48,
    .damage_full_fraction = 0.6,
    .damage_outset = 2.0,
    .damage_pending_max = 16,
    .full_repaint_environment = "LAMINA_FULL_REPAINT",
};

}
