#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/tokens/shape.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/surface.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "lamina_sdl3/window.hpp"

using namespace lamina;
using color::Role;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using tokens::TypeRole;

namespace {

struct Style {
    double page_padding = 24.0;
    double section_gap = 16.0;
    double card_padding = 20.0;
    double card_gap = 12.0;
    double card_elevation = 1.0;
    tokens::CornerSize card_corner = tokens::CornerSize::large;
    Role page = Role::surface;
    Role card = Role::surface_container_low;
};

struct Card {
    std::shared_ptr<layout::Linear> node;
};

}

int main(int argc, char** argv) {
    try {
        const Style style;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const auto scheme = color::spec_2021().build(color::default_scheme_options);

        widgets::Label title("\u914d\u7f6e\u306e\u4ed5\u7d44\u307f", TypeRole::headline_medium, Role::on_surface, fonts);
        widgets::Label intro("\u7a93\u306e\u5927\u304d\u3055\u3092\u5909\u3048\u308b\u3068\u3001\u6587\u7ae0\u306e\u6298\u308a\u8fd4\u3057\u30fb\u30dc\u30bf\u30f3\u306e\u4e26\u3073\u30fb\u5217\u306e\u5e45\u304c\u305d\u306e\u5834\u3067\u7d44\u307f\u76f4\u3055\u308c\u307e\u3059\u3002Rows and columns measure their children first and then place them, like Compose.",
                             TypeRole::body_large, Role::on_surface_variant, fonts);

        widgets::Label left_title("\u5de6\u306e\u5217\uff081\uff09", TypeRole::title_medium, Role::on_surface, fonts);
        widgets::Label left_body("\u91cd\u307f 1 \u306e\u5217\u3067\u3059\u3002\u5e45\u306f\u4f59\u3063\u305f\u5e45\u3092\u91cd\u307f\u306e\u6bd4\u3067\u5206\u3051\u305f\u3082\u306e\u306b\u306a\u308a\u307e\u3059\u3002", TypeRole::body_medium, Role::on_surface_variant, fonts);
        widgets::Label right_title("\u53f3\u306e\u5217\uff082\uff09", TypeRole::title_medium, Role::on_surface, fonts);
        widgets::Label right_body("\u91cd\u307f 2 \u306e\u5217\u3067\u3059\u3002\u5de6\u306e 2 \u500d\u306e\u5e45\u3092\u3082\u3089\u3044\u307e\u3059\u3002Text wraps to whatever width the column receives.", TypeRole::body_medium, Role::on_surface_variant, fonts);

        widgets::TextField name({.label = "\u540d\u524d"}, fonts);
        widgets::Button cancel({"\u30ad\u30e3\u30f3\u30bb\u30eb", ButtonStyle::text, ButtonSize::small, ButtonShape::round}, fonts);
        widgets::Button save({"\u4fdd\u5b58", ButtonStyle::filled, ButtonSize::small, ButtonShape::round}, fonts);

        auto card = [&](std::vector<layout::NodePtr> children) {
            auto c = layout::column(std::move(children));
            c->spacing(style.card_gap).padding(layout::Insets::all(style.card_padding)).cross_align(layout::CrossAlign::stretch);
            return c;
        };
        auto left = card({widgets::node(left_title), widgets::node(left_body)});
        auto right = card({widgets::node(right_title), widgets::node(right_body)});
        auto columns = layout::row({layout::weighted(left, 1.0), layout::weighted(right, 2.0)});
        columns->spacing(style.section_gap).cross_align(layout::CrossAlign::stretch);

        auto actions = layout::row({widgets::node(cancel), widgets::node(save)});
        actions->spacing(style.card_gap).fill_main(true).main_align(layout::MainAlign::end).cross_align(layout::CrossAlign::center);
        auto form = card({widgets::node(name), actions});

        auto page = layout::column({widgets::node(title), widgets::node(intro), columns, form});
        page->spacing(style.section_gap).padding(layout::Insets::all(style.page_padding)).cross_align(layout::CrossAlign::stretch).fill_cross(true);
        form->self_align = layout::CrossAlign::stretch;
        const std::vector<std::shared_ptr<layout::Linear>> cards{left, right, form};

        sdl3::Window window({"lamina-ui layout", 900, 640});
        sdl3::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});

            for (const auto& e : frame.pointer) {
                name.handle(e);
                cancel.handle(e);
                save.handle(e);
            }
            for (const auto& k : frame.keys) {
                name.handle(k);
            }
            for (const auto& t : frame.text) {
                name.handle(t);
            }
            if (name.focused()) {
                const render::Rect area = name.caret_area();
                window.set_text_input(&area);
            } else {
                window.set_text_input(nullptr);
            }

            auto& canvas = window.canvas();
            const color::Argb background = scheme[style.page];
            canvas.clear(background);
            canvas.save();
            canvas.scale(frame.scale);
            for (const auto& c : cards) {
                const render::Rect r = c->rect();
                widgets::paint_surface(canvas, r, tokens::corner_radius(style.card_corner, r.width, r.height), scheme[style.card],
                                       style.card_elevation, scheme[Role::shadow]);
            }
            for (auto* l : {&title, &intro, &left_title, &left_body, &right_title, &right_body}) {
                l->paint(canvas, scheme);
            }
            name.step(frame.dt_seconds);
            name.paint(canvas, scheme, scheme[style.card]);
            animating = name.animating();
            if (name.focused()) {
                window.wake_after(name.next_blink_seconds());
            }
            for (auto* b : {&cancel, &save}) {
                b->step(frame.dt_seconds);
                b->paint(canvas, scheme);
                animating = animating || b->animating();
            }
            canvas.restore();
            window.present();
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
