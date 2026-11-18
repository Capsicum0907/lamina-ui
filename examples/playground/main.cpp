#include <array>
#include <cstdio>
#include <exception>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"

using namespace lamina;
using color::Role;
using tokens::CornerSize;
using tokens::SpringKind;
using tokens::SpringSpeed;

namespace {

struct Layout {
    double margin = 32.0;
    double gap = 24.0;
    int columns = 3;
    int rows = 2;
    double shadow_alpha = 0.3;
    std::size_t rest_elevation = 1;
    std::size_t raised_elevation = 4;
    CornerSize rest_corner = CornerSize::medium;
    CornerSize raised_corner = CornerSize::extra_large;
    Role rest_fill = Role::surface_container_low;
    Role raised_fill = Role::primary_container;
    Role hover_layer = Role::on_surface;
};

struct Card {
    motion::Animated elevation;
    motion::Animated corner;
    motion::Animated fill;
    motion::Animated hover;
    bool selected = false;
};

bool contains(const render::Rect& r, double x, double y) {
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

render::Rect card_rect(const Layout& l, const platform::Frame& f, int index) {
    const double w = (f.width_dp - 2 * l.margin - (l.columns - 1) * l.gap) / l.columns;
    const double h = (f.height_dp - 2 * l.margin - (l.rows - 1) * l.gap) / l.rows;
    const int col = index % l.columns;
    const int row = index / l.columns;
    return {l.margin + col * (w + l.gap), l.margin + row * (h + l.gap), w, h};
}

double corner_dp(CornerSize size) {
    return tokens::corner_radius_dp[static_cast<std::size_t>(size)];
}

}

int main() {
    try {
        const Layout layout;
        const auto& motion_scheme = tokens::expressive_motion;
        const auto spatial = motion_scheme.spring(SpringKind::spatial, SpringSpeed::standard);
        const auto spatial_fast = motion_scheme.spring(SpringKind::spatial, SpringSpeed::fast);
        const auto effects = motion_scheme.spring(SpringKind::effects, SpringSpeed::standard);
        const auto effects_fast = motion_scheme.spring(SpringKind::effects, SpringSpeed::fast);
        const auto scheme = color::spec_2021().build(color::default_scheme_options);

        std::vector<Card> cards;
        for (int i = 0; i < layout.columns * layout.rows; ++i) {
            cards.push_back({
                motion::Animated(tokens::elevation_dp[layout.rest_elevation], spatial),
                motion::Animated(corner_dp(layout.rest_corner), spatial_fast),
                motion::Animated(0.0, effects),
                motion::Animated(0.0, effects_fast),
            });
        }

        platform::Window window({"lamina-ui playground", 960, 600});
        platform::Frame frame;
        double pointer_x = -1.0;
        double pointer_y = -1.0;
        bool animating = true;

        while (window.next_frame(frame, animating)) {
            for (const auto& e : frame.pointer) {
                if (e.action == platform::PointerAction::leave) {
                    pointer_x = pointer_y = -1.0;
                    continue;
                }
                pointer_x = e.x_dp;
                pointer_y = e.y_dp;
                if (e.action != platform::PointerAction::down) {
                    continue;
                }
                for (std::size_t i = 0; i < cards.size(); ++i) {
                    if (!contains(card_rect(layout, frame, static_cast<int>(i)), e.x_dp, e.y_dp)) {
                        continue;
                    }
                    Card& c = cards[i];
                    c.selected = !c.selected;
                    c.elevation.animate_to(tokens::elevation_dp[c.selected ? layout.raised_elevation : layout.rest_elevation]);
                    c.corner.animate_to(corner_dp(c.selected ? layout.raised_corner : layout.rest_corner));
                    c.fill.animate_to(c.selected ? 1.0 : 0.0);
                }
            }

            auto& canvas = window.canvas();
            canvas.clear(scheme[Role::surface]);
            canvas.save();
            canvas.scale(frame.scale);
            for (std::size_t i = 0; i < cards.size(); ++i) {
                Card& c = cards[i];
                const render::Rect r = card_rect(layout, frame, static_cast<int>(i));
                c.hover.animate_to(contains(r, pointer_x, pointer_y) ? tokens::state_layer_opacity::hover : 0.0);
                for (auto* a : {&c.elevation, &c.corner, &c.fill, &c.hover}) {
                    a->step(frame.dt_seconds);
                }
                const auto base = color::mix(scheme[layout.rest_fill], scheme[layout.raised_fill], c.fill.value());
                const auto fill = color::overlay(base, scheme[layout.hover_layer], c.hover.value());
                const double e = c.elevation.value();
                const render::Shadow shadow{0.0, e, e, color::with_alpha(scheme[Role::shadow], layout.shadow_alpha)};
                canvas.fill_path_with_shadow(render::rounded_rect_path(r, render::uniform_radii(c.corner.value())), fill, shadow);
            }
            canvas.restore();
            window.present();

            animating = false;
            for (const Card& c : cards) {
                for (const auto* a : {&c.elevation, &c.corner, &c.fill, &c.hover}) {
                    animating = animating || !a->settled();
                }
            }
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
