#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;

namespace {

struct Layout {
    double margin = 32.0;
    double row_gap = 20.0;
    double column_gap = 16.0;
    double label_width = 120.0;
    color::Role text = color::Role::on_surface;
    color::Role caption = color::Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole caption_type = tokens::TypeRole::label_large;
    double chip = 32.0;
    double chip_gap = 12.0;
    color::Role chip_ring = color::Role::on_surface;
    double chip_ring_width = 2.0;
};

const std::vector<color::Argb> seeds{0xff6750a4, 0xff006a6a, 0xffb3261e, 0xff7d5260, 0xff386a20};

struct Row {
    std::unique_ptr<widgets::Label> caption;
    std::vector<widgets::Button*> buttons;
};

layout::NodePtr caption_cell(widgets::Label& caption, double width, double height, double line_height) {
    caption.measure({});
    return layout::fixed({width, height}, [&caption, line_height](const render::Rect& r) {
        caption.set_origin({r.x, r.y + (r.height - line_height) / 2.0});
    });
}

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        color::SchemeOptions options = color::default_scheme_options;
        color::ColorScheme from = color::spec_2021().build(options);
        color::ColorScheme to = from;
        motion::Animated theme(1.0, tokens::motion_scheme.slow_effects());
        std::size_t seed_index = 0;
        widgets::Button dark_toggle({"ダーク", ButtonStyle::outlined, ButtonSize::extra_small, ButtonShape::round}, fonts);
        widgets::Switch spec_switch;
        widgets::Label spec_label("2025", layout.caption_type, layout.caption, fonts, false);
        auto retheme = [&](const color::ColorScheme& current) {
            from = current;
            to = (spec_switch.checked() ? color::spec_2025() : color::spec_2021()).build(options);
            theme.snap_to(0.0);
            theme.animate_to(1.0);
        };
        const auto caption_style = tokens::text_style(tokens::type_style(layout.caption_type));

        const std::pair<ButtonStyle, const char*> styles[] = {
            {ButtonStyle::filled, "Filled"},     {ButtonStyle::tonal, "Tonal"}, {ButtonStyle::outlined, "Outlined"},
            {ButtonStyle::elevated, "Elevated"}, {ButtonStyle::text, "Text"},
        };
        widgets::WidgetSet controls;
        std::vector<Row> rows;
        for (const auto& [style, name] : styles) {
            Row row{std::make_unique<widgets::Label>(name, layout.caption_type, layout.caption, fonts, false), {}};
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"\u4fdd\u5b58", style, ButtonSize::small, ButtonShape::round}, fonts));
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"Send", style, ButtonSize::small, ButtonShape::square}, fonts));
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"\u7d9a\u3051\u308b", style, ButtonSize::medium, ButtonShape::round}, fonts));
            rows.push_back(std::move(row));
        }
        Row sizes{std::make_unique<widgets::Label>("Sizes", layout.caption_type, layout.caption, fonts, false), {}};
        const std::pair<ButtonSize, const char*> size_list[] = {
            {ButtonSize::extra_small, "XS"}, {ButtonSize::small, "S"}, {ButtonSize::medium, "M"},
            {ButtonSize::large, "L"},        {ButtonSize::extra_large, "XL"},
        };
        for (const auto& [size, name] : size_list) {
            sizes.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{name, ButtonStyle::filled, size, ButtonShape::round}, fonts));
        }
        rows.push_back(std::move(sizes));
        controls.add(dark_toggle);
        controls.add(spec_switch);

        widgets::Label header("\u30dc\u30bf\u30f3", layout.header_type, layout.text, fonts, false);
        int clicks = 0;
        widgets::Label counter("\u62bc\u3057\u305f\u56de\u6570: 0", layout.caption_type, layout.caption, fonts, false);
        widgets::Label theme_caption("Theme", layout.caption_type, layout.caption, fonts, false);

        auto header_row = layout::row({widgets::node(header), widgets::node(counter)});
        header_row->fill_main(true).main_align(layout::MainAlign::space_between);
        header_row->self_align = layout::CrossAlign::stretch;
        std::vector<layout::NodePtr> page_children{header_row};
        for (auto& row : rows) {
            std::vector<layout::NodePtr> items;
            for (auto& b : row.buttons) {
                items.push_back(widgets::node(*b));
            }
            auto buttons_row = layout::row(std::move(items));
            buttons_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
            auto r = layout::row({caption_cell(*row.caption, layout.label_width, caption_style.line_height, caption_style.line_height), buttons_row});
            r->cross_align(layout::CrossAlign::center);
            page_children.push_back(r);
        }
        std::vector<render::Rect> chip_rects(seeds.size());
        std::vector<layout::NodePtr> chips;
        for (std::size_t i = 0; i < seeds.size(); ++i) {
            chips.push_back(layout::fixed({layout.chip, layout.chip}, [&chip_rects, i](const render::Rect& r) { chip_rects[i] = r; }));
        }
        chips.push_back(widgets::node(dark_toggle));
        chips.push_back(widgets::node(spec_label));
        chips.push_back(widgets::node(spec_switch));
        auto chips_row = layout::row(std::move(chips));
        chips_row->spacing(layout.chip_gap).cross_align(layout::CrossAlign::center);
        auto theme_row = layout::row({caption_cell(theme_caption, layout.label_width, layout.chip, caption_style.line_height), chips_row});
        theme_row->cross_align(layout::CrossAlign::center);
        page_children.push_back(theme_row);
        auto page = layout::column(std::move(page_children));
        page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);

        color::ColorScheme scheme = from;
        auto update = [&](const sdl3::Frame& frame) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
            scheme = color::mix(from, to, theme.value());

            for (const auto& e : frame.pointer) {
                if (dark_toggle.handle(e)) {
                    options.dark = !options.dark;
                    retheme(scheme);
                }
                if (spec_switch.handle(e)) {
                    retheme(scheme);
                }
                if (e.action == input::PointerAction::up) {
                    for (std::size_t i = 0; i < seeds.size(); ++i) {
                        const double cx = chip_rects[i].x + chip_rects[i].width / 2.0;
                        const double cy = chip_rects[i].y + chip_rects[i].height / 2.0;
                        if (std::hypot(e.x_dp - cx, e.y_dp - cy) <= layout.chip / 2.0 && i != seed_index) {
                            seed_index = i;
                            options.seed = seeds[i];
                            retheme(scheme);
                        }
                    }
                }
                for (auto& row : rows) {
                    for (auto& b : row.buttons) {
                        if (b->handle(e)) {
                            ++clicks;
                            counter.set_text("押した回数: " + std::to_string(clicks));
                        }
                    }
                }
            }
            return scheme[color::Role::surface];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            header.paint(canvas, scheme);
            counter.paint(canvas, scheme);
            for (auto& row : rows) {
                row.caption->paint(canvas, scheme);
            }
            theme_caption.paint(canvas, scheme);
            spec_label.paint(canvas, scheme);
            for (std::size_t i = 0; i < seeds.size(); ++i) {
                const render::Rect c = chip_rects[i];
                if (i == seed_index) {
                    const render::Rect ring{c.x - 2 * layout.chip_ring_width, c.y - 2 * layout.chip_ring_width,
                                            c.width + 4 * layout.chip_ring_width, c.height + 4 * layout.chip_ring_width};
                    canvas.fill_rounded_rect(ring, render::uniform_radii(ring.width / 2.0), scheme[layout.chip_ring]);
                    const render::Rect gap{c.x - layout.chip_ring_width, c.y - layout.chip_ring_width,
                                           c.width + 2 * layout.chip_ring_width, c.height + 2 * layout.chip_ring_width};
                    canvas.fill_rounded_rect(gap, render::uniform_radii(gap.width / 2.0), scheme[color::Role::surface]);
                }
                canvas.fill_rounded_rect(c, render::uniform_radii(layout.chip / 2.0), seeds[i]);
            }
            theme.step(frame.dt_seconds);
            controls.step(frame.dt_seconds);
            controls.paint(canvas, scheme);
            return !theme.settled() || controls.animating();
        };

        sdl3::run_app({"lamina-ui buttons", 1000, 760}, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
