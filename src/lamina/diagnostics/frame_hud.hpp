#pragma once

#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/frame_stats.hpp"

namespace lamina::diagnostics {

render::Rect paint_frame_hud(render::Canvas& canvas, const text::FontStack& fonts, const color::ColorScheme& scheme,
                             const std::vector<ReportRow>& rows, const std::vector<FrameSample>& history, double refresh_hz,
                             const render::Rect& area,
                             const tokens::FrameStatsTokens& tokens = tokens::frame_stats);

}
