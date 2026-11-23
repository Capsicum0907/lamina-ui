#include "lamina/diagnostics/frame_hud.hpp"

#include <algorithm>
#include <cctype>

#include "lamina/color/mix.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/typography.hpp"

namespace lamina::diagnostics {

namespace {

bool numeric(const std::string& cell) {
    return !cell.empty() && (std::isdigit(static_cast<unsigned char>(cell.front())) != 0 || cell.front() == '-');
}

}

render::Rect paint_frame_hud(render::Canvas& canvas, const text::FontStack& fonts, const color::ColorScheme& scheme,
                             const std::vector<ReportRow>& rows, const std::vector<FrameSample>& history, double refresh_hz,
                             const render::Rect& area, const tokens::FrameStatsTokens& t) {
    const text::TextStyle style = tokens::text_style(tokens::type_style(t.type));
    std::vector<std::vector<text::TextLine>> lines;
    std::vector<double> columns;
    double spanning = 0.0;
    double height = 0.0;
    for (const ReportRow& row : rows) {
        std::vector<text::TextLine> laid;
        double row_height = 0.0;
        for (std::size_t i = 0; i < row.size(); ++i) {
            laid.push_back(text::layout_line(fonts, row[i], style));
            row_height = std::max(row_height, laid.back().box_height());
            if (row.size() == 1) {
                spanning = std::max(spanning, laid.back().width);
            } else {
                columns.resize(std::max(columns.size(), row.size()), 0.0);
                columns[i] = std::max(columns[i], laid.back().width);
            }
        }
        height += row_height;
        lines.push_back(std::move(laid));
    }
    double grid = 0.0;
    for (std::size_t i = 0; i < columns.size(); ++i) {
        grid += columns[i] + (i > 0 ? t.column_gap : 0.0);
    }
    const double bars = static_cast<double>(t.recent_frames) * t.history_bar_width;
    const double width = std::max({spanning, grid, bars}) + 2.0 * t.padding;
    const double box_height = height + t.history_height + 3.0 * t.padding;
    const bool start = t.corner == tokens::ScreenCorner::top_start || t.corner == tokens::ScreenCorner::bottom_start;
    const bool top = t.corner == tokens::ScreenCorner::top_start || t.corner == tokens::ScreenCorner::top_end;
    const double x = start ? area.x + t.margin : area.x + area.width - t.margin - width;
    const double y = top ? area.y + t.margin : area.y + area.height - t.margin - box_height;
    const render::Rect box{x, y, width, box_height};
    canvas.fill_rounded_rect(box, render::uniform_radii(t.corner_radius), color::with_alpha(scheme[t.container], t.container_opacity));
    const double hz = refresh_hz > 0.0 ? refresh_hz : t.fallback_refresh_hz;
    const double base = y + t.padding + t.history_height;
    const double per_refresh = t.history_height / t.history_refreshes;
    for (std::size_t i = 0; i < history.size(); ++i) {
        const FrameSample& f = history[i];
        const double value = f.interval_seconds ? *f.interval_seconds : work_seconds(f);
        const double h = std::min(value * hz * per_refresh, t.history_height);
        const bool late = f.interval_seconds && *f.interval_seconds * hz > t.late_factor;
        canvas.fill_rect({x + t.padding + static_cast<double>(i) * t.history_bar_width, base - h, t.history_bar_width, h},
                         scheme[late ? t.history_late : t.history_bar]);
    }
    canvas.fill_rect({x + t.padding, base - per_refresh, bars, t.history_rule_width}, scheme[t.history_late]);
    const color::Argb ink = scheme[t.content];
    double cy = base + t.padding;
    for (std::size_t r = 0; r < lines.size(); ++r) {
        double cx = x + t.padding;
        double row_height = 0.0;
        for (std::size_t i = 0; i < lines[r].size(); ++i) {
            const text::TextLine& line = lines[r][i];
            row_height = std::max(row_height, line.box_height());
            const bool grid_cell = rows[r].size() > 1;
            const double cell = grid_cell ? columns[i] : line.width;
            const double dx = grid_cell && numeric(rows[r][i]) ? cell - line.width : 0.0;
            text::draw_line(canvas, fonts, line, {cx + dx, cy}, ink);
            cx += cell + t.column_gap;
        }
        cy += row_height;
    }
    return box;
}

}
