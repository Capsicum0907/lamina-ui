#pragma once

#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::input {

enum class CaptionHit { client, drag, minimize, maximize, close };

struct CaptionRegions {
    render::Rect drag{0.0, 0.0, 0.0, 0.0};
    render::Rect minimize{0.0, 0.0, 0.0, 0.0};
    render::Rect maximize{0.0, 0.0, 0.0, 0.0};
    render::Rect close{0.0, 0.0, 0.0, 0.0};
    std::vector<render::Rect> interactive;
    bool report_drag_hover = false;
};

constexpr bool caption_contains(const render::Rect& r, render::Point p) {
    return p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height;
}

inline CaptionHit hit_test(const CaptionRegions& regions, render::Point p_dp) {
    if (caption_contains(regions.maximize, p_dp)) {
        return CaptionHit::maximize;
    }
    if (caption_contains(regions.minimize, p_dp)) {
        return CaptionHit::minimize;
    }
    if (caption_contains(regions.close, p_dp)) {
        return CaptionHit::close;
    }
    for (const render::Rect& r : regions.interactive) {
        if (caption_contains(r, p_dp)) {
            return CaptionHit::client;
        }
    }
    return caption_contains(regions.drag, p_dp) ? CaptionHit::drag : CaptionHit::client;
}

constexpr bool reports_hover(const CaptionRegions& regions, CaptionHit hit) {
    return hit == CaptionHit::maximize || (hit == CaptionHit::drag && regions.report_drag_hover);
}

}
