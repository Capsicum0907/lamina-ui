#include <cstdio>
#include <exception>
#include <memory>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/shapes/material_shapes.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/to_path.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina_sdl3/app.hpp"

using namespace lamina;
using shapes::MaterialShape;

namespace {

struct Layout {
    double margin = 32.0;
    int columns = 7;
    double cell = 64.0;
    double cell_inset = 8.0;
    double stage = 320.0;
    double stage_gap = 48.0;
    color::Role background = color::Role::surface;
    color::Role cell_fill = color::Role::secondary_container;
    color::Role cell_selected = color::Role::primary;
    color::Role stage_fill = color::Role::primary;
    color::Role hover = color::Role::on_surface;
};

render::Path placed(const render::Path& unit, const render::Rect& r) {
    return unit.transformed({r.width, 0.0, 0.0, r.height, r.x, r.y});
}

}

int main() {
    try {
        const Layout layout;
        const auto scheme = color::spec_2021().build(color::default_scheme_options);
        const auto& all = shapes::all_material_shapes();
        const int rows = static_cast<int>((all.size() + layout.columns - 1) / layout.columns);

        std::size_t current = 0;
        std::size_t previous = 0;
        auto morph = std::make_unique<shapes::Morph>(shapes::material_shape(all[previous]), shapes::material_shape(all[current]));
        motion::Animated progress(1.0, tokens::motion_scheme.default_spatial());
        std::size_t hovered = all.size();

        std::vector<render::Rect> cells(all.size());
        render::Rect stage{0.0, 0.0, 0.0, 0.0};
        std::vector<layout::NodePtr> grid_rows;
        for (int r = 0; r < rows; ++r) {
            std::vector<layout::NodePtr> row_cells;
            for (int col = 0; col < layout.columns; ++col) {
                const auto i = static_cast<std::size_t>(r * layout.columns + col);
                if (i < all.size()) {
                    row_cells.push_back(layout::fixed({layout.cell, layout.cell}, [&cells, i](const render::Rect& rect) { cells[i] = rect; }));
                }
            }
            grid_rows.push_back(layout::row(std::move(row_cells)));
        }
        auto stage_node = layout::fixed({layout.stage, layout.stage}, [&stage](const render::Rect& rect) { stage = rect; });
        auto page = layout::row({layout::column(std::move(grid_rows)), stage_node});
        page->spacing(layout.stage_gap).padding(layout::Insets::all(layout.margin)).cross_align(layout::CrossAlign::center);
        const layout::Size natural = page->measure({});

        auto update = [&](const sdl3::Frame& frame) {
            layout::run(*page, {0.0, 0.0, natural.width, natural.height});
            for (const auto& e : frame.pointer) {
                hovered = all.size();
                for (std::size_t i = 0; i < all.size(); ++i) {
                    if (e.action != input::PointerAction::leave && widgets::contains(cells[i], e.x_dp, e.y_dp)) {
                        hovered = i;
                    }
                }
                if (e.action == input::PointerAction::down && hovered < all.size() && hovered != current) {
                    previous = current;
                    current = hovered;
                    morph = std::make_unique<shapes::Morph>(shapes::material_shape(all[previous]), shapes::material_shape(all[current]));
                    progress.snap_to(0.0);
                    progress.animate_to(1.0);
                }
            }
            return scheme[layout.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            for (std::size_t i = 0; i < all.size(); ++i) {
                const render::Rect c = cells[i];
                const render::Rect r{c.x + layout.cell_inset, c.y + layout.cell_inset, c.width - 2 * layout.cell_inset,
                                     c.height - 2 * layout.cell_inset};
                color::Argb fill = scheme[i == current ? layout.cell_selected : layout.cell_fill];
                if (i == hovered && i != current) {
                    fill = scheme[layout.stage_fill];
                }
                canvas.fill_path(placed(shapes::to_path(shapes::material_shape(all[i])), r), fill);
            }
            progress.step(frame.dt_seconds);
            canvas.fill_path(placed(shapes::to_path(*morph, static_cast<float>(progress.value())), stage), scheme[layout.stage_fill]);
            return !progress.settled();
        };

        sdl3::run_app({"lamina-ui shapes", static_cast<int>(natural.width), static_cast<int>(natural.height)}, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
