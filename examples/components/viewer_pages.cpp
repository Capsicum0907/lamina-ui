#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/pager.hpp"
#include "lamina/widgets/zoom_view.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::components {

namespace {

struct PhotoShape {
    double width;
    double height;
};

struct ViewerLayout {
    double max_width = 720.0;
    double aspect = 0.625;
    double photo_aspect = 0.75;
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
    std::size_t photo_count = 12;
    std::array<PhotoShape, 3> photo_shapes{{{1600.0, 1000.0}, {900.0, 1400.0}, {1200.0, 1200.0}}};
    std::array<color::Role, 4> photo_grounds{color::Role::primary_container, color::Role::secondary_container, color::Role::tertiary_container,
                                             color::Role::surface_container_high};
    std::array<color::Role, 4> photo_figures{color::Role::primary, color::Role::secondary, color::Role::tertiary, color::Role::outline};
    std::array<color::Role, 4> photo_marks{color::Role::on_primary, color::Role::on_secondary, color::Role::on_tertiary, color::Role::surface};
    color::Role photo_stage = color::Role::surface_container_lowest;
    int photo_bands = 8;
    double photo_band_fraction = 0.5;
    double photo_disc_fraction = 0.32;
    double photo_frame_fraction = 0.06;
    double photo_frame_width_fraction = 0.012;
    tokens::TypeRole photo_number_type = tokens::TypeRole::display_large;
    double photo_number_fraction = 0.28;
};

const ViewerLayout viewer_layout;

constexpr const char* view_name = "Pattern";
constexpr const char* photos_name = "Photos";
constexpr const char* photo_prefix = "Photo ";
constexpr const char* photo_report = "Photo: ";
constexpr const char* photo_of = " / ";

std::string percent(double scale) {
    return std::to_string(static_cast<long long>(std::lround(scale * viewer_layout.percent))) + " %";
}

const PhotoShape& photo_shape(std::size_t index) {
    return viewer_layout.photo_shapes[index % viewer_layout.photo_shapes.size()];
}

class ViewerPage final : public BasePage {
public:
    explicit ViewerPage(PageContext context)
        : BasePage(std::move(context)),
          zoom_({.accessible_name = view_name, .content_width = viewer_layout.content_width, .content_height = viewer_layout.content_height}),
          photos_({.count = viewer_layout.photo_count, .accessible_name = photos_name}) {
        const ViewerLayout& l = viewer_layout;
        auto& fit = controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    zoom_.reset();
                }
            },
            widgets::ButtonOptions{"Fit", tokens::ButtonStyle::tonal}, ctx.fonts);
        focus_.add(fit);
        focus_.add(photos_);
        enabler("Fit", fit);
        auto view = std::make_shared<layout::Leaf>(
            [](const layout::Constraints& c) {
                const double w = std::min(viewer_layout.max_width, c.max_width);
                return layout::Size{w, w * viewer_layout.aspect};
            },
            [this](const render::Rect& r) { zoom_.set_bounds(r); });
        auto photos = std::make_shared<layout::Leaf>(
            [](const layout::Constraints& c) {
                const double w = std::min(viewer_layout.max_width, c.max_width);
                return layout::Size{w, w * viewer_layout.photo_aspect};
            },
            [this](const render::Rect& r) {
                photos_.set_bounds(r);
                place_photos();
            });
        std::vector<layout::NodePtr> rows;
        rows.push_back(captioned("Pinch to zoom", view));
        rows.push_back(captioned("Reset", widgets::centered_node(fit)));
        rows.push_back(captioned("Swipe photos", photos));
        auto column = layout::column(std::move(rows));
        column->spacing(l.row_gap);
        root = heading("Zoom view", column);
        place_photos();
    }

    bool handle(const input::FingerEvent& e) override {
        bool used = zoom_.handle(e);
        if (widgets::ZoomView* p = photo(photos_.settled_page())) {
            used = p->handle(e) || used;
        }
        return used;
    }
    bool handle(const input::PointerEvent& e) override {
        const bool zoomed = zoom_.handle(e);
        if (zoom_.holds_pointer()) {
            return true;
        }
        const bool paged = photos_.route(e, [this](const input::PointerEvent& pe) {
            widgets::ZoomView* p = photo(photos_.settled_page());
            if (!p) {
                return false;
            }
            p->handle(pe);
            return p->holds_pointer();
        });
        place_photos();
        if (paged || photos_.holds_pointer() || photo_holds()) {
            return true;
        }
        return BasePage::handle(e) || zoomed;
    }
    bool handle(const input::KeyEvent& e) override { return photos_.handle(e) || BasePage::handle(e); }
    bool handle(const input::WheelEvent& e) override {
        if (zoom_.handle(e)) {
            return true;
        }
        widgets::ZoomView* p = photo(photos_.settled_page());
        return p && !photos_.scroll_in_progress() && p->handle(e);
    }
    bool holds_pointer() const override { return zoom_.holds_pointer() || photos_.holds_pointer() || photo_holds() || BasePage::holds_pointer(); }

    void step(double dt) override {
        BasePage::step(dt);
        zoom_.step(dt);
        const std::size_t before = photos_.settled_page();
        photos_.step(dt);
        place_photos();
        for (auto& [index, p] : photo_views_) {
            p->step(dt);
        }
        if (photos_.settled_page() != before) {
            for (auto& [index, p] : photo_views_) {
                if (index != photos_.settled_page()) {
                    p->snap_to_fit();
                }
            }
            ctx.report(photo_report + std::to_string(photos_.settled_page() + 1) + photo_of + std::to_string(photos_.count()));
        }
        drop_far_photos();
        const bool resting = !zoom_.animating() && !zoom_.gesture_active();
        if (resting && std::abs(zoom_.scale() - reported_) > zoom_.tokens().zoomed_epsilon) {
            reported_ = zoom_.scale();
            ctx.report(std::string("Zoom: ") + percent(reported_));
        }
    }
    bool animating() const override {
        if (BasePage::animating() || zoom_.animating() || photos_.animating()) {
            return true;
        }
        return std::any_of(photo_views_.begin(), photo_views_.end(), [](const auto& p) { return p.second->animating(); });
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint(canvas, scheme);
        const ViewerLayout& l = viewer_layout;
        const render::Rect& r = zoom_.bounds();
        const double radius = current_style().shape.radius(l.corner, r.width, r.height);
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
        paint_photos(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        if (auto n = zoom_.accessible()) {
            nodes.push_back(std::move(*n));
        }
        widgets::AccessibleNode pager = photos_.accessible();
        if (const widgets::ZoomView* p = photo(photos_.settled_page())) {
            if (auto n = p->accessible()) {
                pager.children.push_back(std::move(*n));
            }
        }
        nodes.push_back(std::move(pager));
        return nodes;
    }
    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        zoom_.set_enabled(enabled);
        photos_.set_enabled(enabled);
        for (auto& [index, p] : photo_views_) {
            p->set_enabled(enabled);
        }
    }
    void cancel_interaction() override {
        BasePage::cancel_interaction();
        zoom_.cancel_interaction();
        photos_.cancel_interaction();
        for (auto& [index, p] : photo_views_) {
            p->cancel_interaction();
        }
    }
    std::optional<input::Cursor> cursor() const override {
        if (auto c = zoom_.cursor()) {
            return c;
        }
        const widgets::ZoomView* p = photo(photos_.settled_page());
        return p ? p->cursor() : std::nullopt;
    }

private:
    widgets::ZoomView* photo(std::size_t index) {
        const auto it = photo_views_.find(index);
        return it == photo_views_.end() ? nullptr : it->second.get();
    }
    const widgets::ZoomView* photo(std::size_t index) const {
        const auto it = photo_views_.find(index);
        return it == photo_views_.end() ? nullptr : it->second.get();
    }
    bool photo_holds() const {
        const widgets::ZoomView* p = photo(photos_.settled_page());
        return p && p->holds_pointer();
    }

    void place_photos() {
        const motion::PageRange wanted = photos_.pages_to_build();
        for (std::size_t i = wanted.first; !wanted.empty && i <= wanted.last; ++i) {
            if (!photo(i)) {
                const PhotoShape& shape = photo_shape(i);
                auto view = std::make_unique<widgets::ZoomView>(widgets::ZoomViewOptions{
                    .accessible_name = photo_prefix + std::to_string(i + 1), .content_width = shape.width, .content_height = shape.height});
                view->set_enabled(photos_.enabled());
                photo_views_.emplace(i, std::move(view));
            }
            photo(i)->set_bounds(photos_.page_rect(i));
        }
    }

    void drop_far_photos() {
        const motion::PageRange wanted = photos_.pages_to_build();
        std::erase_if(photo_views_, [&wanted](const auto& p) { return !wanted.contains(p.first); });
    }

    void paint_photos(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        const ViewerLayout& l = viewer_layout;
        const render::Rect& r = photos_.bounds();
        const double radius = current_style().shape.radius(l.corner, r.width, r.height);
        canvas.save();
        canvas.push_clip(render::rounded_rect_path(r, render::uniform_radii(radius)));
        canvas.fill_rect(r, scheme[l.photo_stage]);
        const motion::PageRange shown = photos_.pages_in_view();
        for (std::size_t i = shown.first; !shown.empty && i <= shown.last; ++i) {
            const widgets::ZoomView* p = photo(i);
            if (!p) {
                continue;
            }
            p->begin_content(canvas);
            paint_photo(canvas, scheme, i);
            p->end_content(canvas);
        }
        canvas.pop_clip();
        canvas.restore();
        canvas.fill_path(render::ring_path(r, radius, l.border_width), scheme[l.border], render::FillRule::even_odd);
        photos_.paint_focus(canvas, scheme);
    }

    void paint_photo(render::Canvas& canvas, const color::ColorScheme& scheme, std::size_t index) const {
        const ViewerLayout& l = viewer_layout;
        const PhotoShape& shape = photo_shape(index);
        const color::Argb ground = scheme[l.photo_grounds[index % l.photo_grounds.size()]];
        const color::Argb figure = scheme[l.photo_figures[index % l.photo_figures.size()]];
        canvas.fill_rect({0.0, 0.0, shape.width, shape.height}, ground);
        const double band = shape.height / l.photo_bands;
        for (int k = 0; k < l.photo_bands; k += 2) {
            canvas.fill_rect({0.0, k * band, shape.width, band * l.photo_band_fraction}, color::with_alpha(figure, l.photo_band_fraction));
        }
        const double side = std::min(shape.width, shape.height);
        const render::Point centre{shape.width / 2.0, shape.height / 2.0};
        canvas.fill_path(render::circle_path(centre, side * l.photo_disc_fraction), figure);
        const double inset = side * l.photo_frame_fraction;
        const render::Rect frame{inset, inset, shape.width - 2.0 * inset, shape.height - 2.0 * inset};
        canvas.fill_path(render::ring_path(frame, inset, side * l.photo_frame_width_fraction), figure, render::FillRule::even_odd);
        const auto style = current_style().type.text(l.photo_number_type);
        const text::TextLine number = text::layout_line(ctx.fonts, std::to_string(index + 1), style);
        const double grow = side * l.photo_number_fraction / std::max(1.0, number.box_height());
        canvas.save();
        canvas.translate(centre.x, centre.y);
        canvas.scale(grow);
        text::draw_line(canvas, ctx.fonts, number, {-number.width / 2.0, -number.box_height() / 2.0}, scheme[l.photo_marks[index % l.photo_marks.size()]]);
        canvas.restore();
    }

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
    widgets::Pager photos_;
    std::map<std::size_t, std::unique_ptr<widgets::ZoomView>> photo_views_;
    double reported_ = 1.0;
};

}

std::unique_ptr<Page> make_viewer_page(PageContext context) {
    return std::make_unique<ViewerPage>(std::move(context));
}

}
