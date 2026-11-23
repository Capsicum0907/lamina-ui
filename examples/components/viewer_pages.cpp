#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/zoom_view.hpp"

namespace lamina::examples::components {

namespace {

struct ViewerLayout {
    double max_width = 720.0;
    double aspect = 0.625;
    double row_gap = 24.0;
    double content_width = 1600.0;
    double content_height = 1000.0;
    int columns = 16;
    int rows = 10;
    double tile_gap = 6.0;
    double tile_corner = 12.0;
    double ring_step = 80.0;
    int rings = 6;
    double ring_width = 14.0;
    int fine_lines = 10;
    double fine_line_width = 1.0;
    double marker_radius = 28.0;
    double marker_inset = 60.0;
    double percent = 100.0;
    tokens::CornerSize corner = tokens::CornerSize::large;
    double border_width = 1.0;
    color::Role background = color::Role::surface_container_highest;
    color::Role tile = color::Role::primary_container;
    color::Role alternate_tile = color::Role::tertiary_container;
    color::Role ring = color::Role::tertiary;
    color::Role alternate_ring = color::Role::on_tertiary_container;
    color::Role fine_line = color::Role::on_primary_container;
    color::Role marker = color::Role::error;
    color::Role border = color::Role::outline_variant;
};

const ViewerLayout viewer_layout;

constexpr const char* view_name = "Pattern";

std::string percent(double scale) {
    return std::to_string(static_cast<long long>(std::lround(scale * viewer_layout.percent))) + " %";
}

class ViewerPage final : public BasePage {
public:
    explicit ViewerPage(PageContext context)
        : BasePage(std::move(context)),
          zoom_({.accessible_name = view_name, .content_width = viewer_layout.content_width, .content_height = viewer_layout.content_height}) {
        const ViewerLayout& l = viewer_layout;
        auto& fit = controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    zoom_.reset();
                }
            },
            widgets::ButtonOptions{"Fit", tokens::ButtonStyle::tonal}, ctx.fonts);
        focus_.add(fit);
        enabler("Fit", fit);
        auto view = std::make_shared<layout::Leaf>(
            [](const layout::Constraints& c) {
                const double w = std::min(viewer_layout.max_width, c.max_width);
                return layout::Size{w, w * viewer_layout.aspect};
            },
            [this](const render::Rect& r) { zoom_.set_bounds(r); });
        std::vector<layout::NodePtr> rows;
        rows.push_back(captioned("Pinch to zoom", view));
        rows.push_back(captioned("Reset", widgets::centered_node(fit)));
        auto column = layout::column(std::move(rows));
        column->spacing(l.row_gap);
        root = heading("Zoom view", column);
    }

    bool handle(const input::FingerEvent& e) override { return zoom_.handle(e); }
    bool handle(const input::PointerEvent& e) override {
        const bool zoomed = zoom_.handle(e);
        if (zoom_.holds_pointer()) {
            return true;
        }
        return BasePage::handle(e) || zoomed;
    }
    bool handle(const input::WheelEvent& e) override { return zoom_.handle(e); }
    bool holds_pointer() const override { return zoom_.holds_pointer() || BasePage::holds_pointer(); }

    void step(double dt) override {
        BasePage::step(dt);
        zoom_.step(dt);
        const bool resting = !zoom_.animating() && !zoom_.gesture_active();
        if (resting && std::abs(zoom_.scale() - reported_) > zoom_.tokens().zoomed_epsilon) {
            reported_ = zoom_.scale();
            ctx.report(std::string("Zoom: ") + percent(reported_));
        }
    }
    bool animating() const override { return BasePage::animating() || zoom_.animating(); }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint(canvas, scheme);
        const ViewerLayout& l = viewer_layout;
        const render::Rect& r = zoom_.bounds();
        const double radius = tokens::corner_radius(l.corner, r.width, r.height);
        const render::Path outline = render::rounded_rect_path(r, render::uniform_radii(radius));
        canvas.save();
        canvas.push_clip(outline);
        canvas.fill_rect(r, scheme[l.background]);
        zoom_.begin_content(canvas);
        paint_pattern(canvas, scheme);
        zoom_.end_content(canvas);
        canvas.pop_clip();
        canvas.restore();
        canvas.fill_path(render::ring_path(r, radius, l.border_width), scheme[l.border], render::FillRule::even_odd);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        if (auto n = zoom_.accessible()) {
            nodes.push_back(std::move(*n));
        }
        return nodes;
    }
    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        zoom_.set_enabled(enabled);
    }
    void cancel_interaction() override {
        BasePage::cancel_interaction();
        zoom_.cancel_interaction();
    }
    std::optional<input::Cursor> cursor() const override { return zoom_.cursor(); }

private:
    static void paint_pattern(render::Canvas& canvas, const color::ColorScheme& scheme) {
        const ViewerLayout& l = viewer_layout;
        const double tw = l.content_width / l.columns;
        const double th = l.content_height / l.rows;
        canvas.fill_rect({0.0, 0.0, l.content_width, l.content_height}, scheme[l.background]);
        for (int j = 0; j < l.rows; ++j) {
            for (int i = 0; i < l.columns; ++i) {
                const render::Rect tile{i * tw + l.tile_gap, j * th + l.tile_gap, tw - 2.0 * l.tile_gap, th - 2.0 * l.tile_gap};
                canvas.fill_rounded_rect(tile, render::uniform_radii(l.tile_corner), scheme[(i + j) % 2 == 0 ? l.tile : l.alternate_tile]);
            }
        }
        const render::Point centre{l.content_width / 2.0, l.content_height / 2.0};
        for (int k = 1; k <= l.rings; ++k) {
            const double radius = k * l.ring_step;
            canvas.fill_path(render::ring_path(render::circle_rect(centre, radius), radius, l.ring_width), scheme[k % 2 == 0 ? l.ring : l.alternate_ring],
                             render::FillRule::even_odd);
        }
        const render::Rect fine{centre.x - tw / 2.0, centre.y - th / 2.0, tw, th};
        for (int k = 0; k <= l.fine_lines; ++k) {
            const double fx = fine.x + fine.width * k / l.fine_lines;
            const double fy = fine.y + fine.height * k / l.fine_lines;
            canvas.fill_rect({fx - l.fine_line_width / 2.0, fine.y, l.fine_line_width, fine.height}, scheme[l.fine_line]);
            canvas.fill_rect({fine.x, fy - l.fine_line_width / 2.0, fine.width, l.fine_line_width}, scheme[l.fine_line]);
        }
        const std::array<render::Point, 4> corners{render::Point{l.marker_inset, l.marker_inset},
                                                   render::Point{l.content_width - l.marker_inset, l.marker_inset},
                                                   render::Point{l.marker_inset, l.content_height - l.marker_inset},
                                                   render::Point{l.content_width - l.marker_inset, l.content_height - l.marker_inset}};
        for (const render::Point& p : corners) {
            canvas.fill_path(render::circle_path(p, l.marker_radius), scheme[l.marker]);
        }
    }

    widgets::ZoomView zoom_;
    double reported_ = 1.0;
};

}

std::unique_ptr<Page> make_viewer_page(PageContext context) {
    return std::make_unique<ViewerPage>(std::move(context));
}

}
