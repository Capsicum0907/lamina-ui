#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "common/canvas.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/surface.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using color::Role;
using tokens::CornerSize;

namespace {

struct Layout {
    double margin = 32.0;
    double header_height = 64.0;
    double card_padding = 20.0;
    double title_gap = 4.0;
    double gap = 24.0;
    int columns = 3;
    int rows = 2;
    std::size_t rest_elevation = 1;
    std::size_t raised_elevation = 4;
    CornerSize rest_corner = CornerSize::medium;
    CornerSize raised_corner = CornerSize::extra_large;
    Role rest_fill = Role::surface_container_low;
    Role raised_fill = Role::primary_container;
    Role hover_layer = Role::on_surface;
    Role header_text = Role::on_surface;
    Role rest_text = Role::on_surface;
    Role raised_text = Role::on_primary_container;
    Role body_text = Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole title_type = tokens::TypeRole::title_medium;
    tokens::TypeRole body_type = tokens::TypeRole::body_medium;
};

const char* header_text = "lamina-ui の遊び場";
const char* body_text = "クリックで浮き上がります。高さ・角の丸み・色が、それぞればねで動きます。Springs drive every change, so the motion settles naturally.";

struct Card {
    motion::Animated elevation;
    motion::Animated corner;
    motion::Animated fill;
    motion::Animated hover;
    bool selected = false;
    render::Rect rect{0.0, 0.0, 0.0, 0.0};
};

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const auto header_style = tokens::text_style(tokens::type_style(layout.header_type));
        const auto title_style = tokens::text_style(tokens::type_style(layout.title_type));
        const auto body_style = tokens::text_style(tokens::type_style(layout.body_type));
        const auto header_line = text::layout_line(fonts, header_text, header_style);
        double body_width = -1.0;
        text::TextBlock body_block;
        std::vector<text::TextLine> titles;
        const auto spatial = tokens::motion_scheme.default_spatial();
        const auto spatial_fast = tokens::motion_scheme.fast_spatial();
        const auto effects = tokens::motion_scheme.default_effects();
        const auto effects_fast = tokens::motion_scheme.fast_effects();
        const auto scheme = color::spec_2021().build(color::default_scheme_options);

        std::vector<Card> cards;
        for (int i = 0; i < layout.columns * layout.rows; ++i) {
            cards.push_back({
                motion::Animated(tokens::elevation_dp[layout.rest_elevation], spatial),
                motion::Animated(tokens::corner_radius(layout.rest_corner, 0.0, 0.0), spatial_fast),
                motion::Animated(0.0, effects),
                motion::Animated(0.0, effects_fast),
            });
            titles.push_back(text::layout_line(fonts, "カード " + std::to_string(i + 1), title_style));
        }

        render::Point header_at{0.0, 0.0};
        std::vector<layout::NodePtr> grid_rows;
        for (int r = 0; r < layout.rows; ++r) {
            std::vector<layout::NodePtr> row_cards;
            for (int col = 0; col < layout.columns; ++col) {
                Card& c = cards[static_cast<std::size_t>(r * layout.columns + col)];
                row_cards.push_back(layout::weighted(layout::fixed({0.0, 0.0}, [&c](const render::Rect& rect) { c.rect = rect; }), 1.0));
            }
            auto row = layout::row(std::move(row_cards));
            row->spacing(layout.gap).cross_align(layout::CrossAlign::stretch).fill_cross(true);
            grid_rows.push_back(layout::weighted(row, 1.0));
        }
        auto grid = layout::column(std::move(grid_rows));
        grid->spacing(layout.gap).cross_align(layout::CrossAlign::stretch).fill_cross(true);
        auto header = layout::fixed({0.0, layout.header_height}, [&header_at](const render::Rect& r) { header_at = {r.x, r.y}; });
        auto page = layout::column({header, layout::weighted(grid, 1.0)});
        page->padding(layout::Insets::all(layout.margin)).cross_align(layout::CrossAlign::stretch).fill_cross(true);

        double pointer_x = -1.0;
        double pointer_y = -1.0;

        auto update = [&](const sdl3::Frame& frame) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
            for (const auto& e : frame.pointer) {
                if (e.action == input::PointerAction::leave) {
                    pointer_x = pointer_y = -1.0;
                    continue;
                }
                pointer_x = e.x_dp;
                pointer_y = e.y_dp;
                if (e.action != input::PointerAction::down) {
                    continue;
                }
                for (Card& c : cards) {
                    if (!widgets::contains(c.rect, e.x_dp, e.y_dp)) {
                        continue;
                    }
                    c.selected = !c.selected;
                    c.elevation.animate_to(tokens::elevation_dp[c.selected ? layout.raised_elevation : layout.rest_elevation]);
                    c.corner.animate_to(tokens::corner_radius(c.selected ? layout.raised_corner : layout.rest_corner, c.rect.width, c.rect.height));
                    c.fill.animate_to(c.selected ? 1.0 : 0.0);
                }
            }
            return scheme[Role::surface];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            text::draw_line(canvas, fonts, header_line, header_at, scheme[layout.header_text]);
            for (std::size_t i = 0; i < cards.size(); ++i) {
                Card& c = cards[i];
                const render::Rect r = c.rect;
                c.hover.animate_to(widgets::contains(r, pointer_x, pointer_y) ? tokens::state_layer_opacity::hover : 0.0);
                for (auto* a : {&c.elevation, &c.corner, &c.fill, &c.hover}) {
                    a->step(frame.dt_seconds);
                }
                const auto base = color::mix(scheme[layout.rest_fill], scheme[layout.raised_fill], c.fill.value());
                const auto fill = color::overlay(base, scheme[layout.hover_layer], c.hover.value());
                widgets::paint_surface(canvas, r, c.corner.value(), fill, c.elevation.value(), scheme[Role::shadow]);
                const render::Point title_at{r.x + layout.card_padding, r.y + layout.card_padding};
                const auto title_color = color::mix(scheme[layout.rest_text], scheme[layout.raised_text], c.fill.value());
                text::draw_line(canvas, fonts, titles[i], title_at, title_color);
                const double wrap = r.width - 2 * layout.card_padding;
                if (wrap != body_width) {
                    body_width = wrap;
                    body_block = text::layout_paragraph(fonts, body_text, body_style, wrap);
                }
                text::draw_block(canvas, fonts, body_block,
                                 {title_at.x, title_at.y + title_style.line_height + layout.title_gap}, scheme[layout.body_text]);
            }
            bool animating = false;
            for (const Card& c : cards) {
                for (const auto* a : {&c.elevation, &c.corner, &c.fill, &c.hover}) {
                    animating = animating || !a->settled();
                }
            }
            return animating;
        };

        sdl3::run_app({"lamina-ui playground", 960, 600}, examples::window_canvas(), update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
