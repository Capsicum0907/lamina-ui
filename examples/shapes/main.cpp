#include <cstdio>
#include <exception>
#include <memory>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/shapes/material_shapes.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/shapes/to_path.hpp"
#include "lamina/tokens/motion.hpp"

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

bool inside(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
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
        motion::Animated progress(1.0, tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard));
        std::size_t hovered = all.size();

        auto cell_rect = [&](std::size_t i) {
            const int col = static_cast<int>(i) % layout.columns;
            const int row = static_cast<int>(i) / layout.columns;
            return render::Rect{layout.margin + col * layout.cell, layout.margin + row * layout.cell, layout.cell, layout.cell};
        };
        const render::Rect stage{layout.margin + layout.columns * layout.cell + layout.stage_gap,
                                 layout.margin + (rows * layout.cell - layout.stage) / 2.0, layout.stage, layout.stage};

        platform::Window window({"lamina-ui shapes", static_cast<int>(stage.x + stage.width + layout.margin),
                                 static_cast<int>(layout.margin * 2 + rows * layout.cell)});
        platform::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            for (const auto& e : frame.pointer) {
                hovered = all.size();
                for (std::size_t i = 0; i < all.size(); ++i) {
                    if (e.action != platform::PointerAction::leave && inside(cell_rect(i), e.x_dp, e.y_dp)) {
                        hovered = i;
                    }
                }
                if (e.action == platform::PointerAction::down && hovered < all.size() && hovered != current) {
                    previous = current;
                    current = hovered;
                    morph = std::make_unique<shapes::Morph>(shapes::material_shape(all[previous]), shapes::material_shape(all[current]));
                    progress.snap_to(0.0);
                    progress.animate_to(1.0);
                }
            }

            auto& canvas = window.canvas();
            canvas.clear(scheme[layout.background]);
            canvas.save();
            canvas.scale(frame.scale);
            for (std::size_t i = 0; i < all.size(); ++i) {
                const render::Rect c = cell_rect(i);
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
            canvas.restore();
            window.present();
            animating = !progress.settled();
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
