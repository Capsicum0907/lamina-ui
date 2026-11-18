#include <array>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/elevation.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/tokens/typography.hpp"

using namespace lamina;
using color::Role;
using tokens::CornerSize;
using tokens::SpringKind;
using tokens::SpringSpeed;

namespace {

struct Layout {
    double margin = 32.0;
    double header_height = 64.0;
    double card_padding = 20.0;
    double title_gap = 4.0;
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
    Role header_text = Role::on_surface;
    Role rest_text = Role::on_surface;
    Role raised_text = Role::on_primary_container;
    Role body_text = Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole title_type = tokens::TypeRole::title_medium;
    tokens::TypeRole body_type = tokens::TypeRole::body_medium;
};

const std::vector<std::string> default_fonts{"C:/Windows/Fonts/NotoSansJP-VF.ttf"};
const char* header_text = "lamina-ui の遊び場";
const char* body_text = "クリックで浮き上がります";

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
    const double h = (f.height_dp - 2 * l.margin - l.header_height - (l.rows - 1) * l.gap) / l.rows;
    const int col = index % l.columns;
    const int row = index / l.columns;
    return {l.margin + col * (w + l.gap), l.margin + l.header_height + row * (h + l.gap), w, h};
}

double corner_dp(CornerSize size) {
    return tokens::corner_radius_dp[static_cast<std::size_t>(size)];
}

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        text::FontStack fonts;
        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                fonts.add(text::Font::from_file(argv[i]));
            }
        } else {
            for (const auto& path : default_fonts) {
                fonts.add(text::Font::from_file(path));
            }
        }
        const auto header_style = text::text_style(tokens::type_style(layout.header_type));
        const auto title_style = text::text_style(tokens::type_style(layout.title_type));
        const auto body_style = text::text_style(tokens::type_style(layout.body_type));
        const auto header_line = text::layout_line(fonts, header_text, header_style);
        const auto body_line = text::layout_line(fonts, body_text, body_style);
        std::vector<text::TextLine> titles;
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
            titles.push_back(text::layout_line(fonts, "カード " + std::to_string(i + 1), title_style));
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
            text::draw_line(canvas, fonts, header_line, {layout.margin, layout.margin}, scheme[layout.header_text]);
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
                const render::Point title_at{r.x + layout.card_padding, r.y + layout.card_padding};
                const auto title_color = color::mix(scheme[layout.rest_text], scheme[layout.raised_text], c.fill.value());
                text::draw_line(canvas, fonts, titles[i], title_at, title_color);
                text::draw_line(canvas, fonts, body_line,
                                {title_at.x, title_at.y + title_style.line_height + layout.title_gap}, scheme[layout.body_text]);
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
