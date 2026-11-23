#include "lamina_sdl3/frame_stats.hpp"

#include <string>

#include "lamina/diagnostics/frame_hud.hpp"
#include "lamina/layout/safe_area.hpp"

namespace lamina::sdl3 {

std::optional<render::Rect> paint_frame_stats(Window& window, render::Canvas& canvas, const text::FontStack& fonts, const color::ColorScheme& scheme,
                                              const Frame& frame) {
    diagnostics::FrameProbe& probe = window.frame_probe();
    if (!probe.switches().hud) {
        return std::nullopt;
    }
    const diagnostics::StageScope stage(&probe, diagnostics::Stage::overlay);
    const diagnostics::FrameSummary summary = probe.recent();
    const std::string title = "last " + std::to_string(probe.tokens().recent_frames);
    const render::Rect area = layout::inset_rect({0.0, 0.0, frame.width_dp, frame.height_dp}, frame.safe_area);
    return diagnostics::paint_frame_hud(canvas, fonts, scheme, diagnostics::report_rows(summary, probe.display(), title, probe.tokens()),
                                        probe.history(), probe.display().refresh_hz, area, probe.tokens());
}

}
