#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "common/canvas.hpp"
#include "common/caption_cell.hpp"
#include "common/theme.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/text/layout.hpp"
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
};

struct Row {
    std::unique_ptr<widgets::Label> caption;
    std::vector<widgets::Button*> buttons;
};

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        color::SchemeOptions options = color::default_scheme_options;
        examples::ThemeFade theme(color::spec_2021().build(options));
        examples::SeedChips chips;
        widgets::Button dark_toggle({"ダーク", ButtonStyle::outlined, ButtonSize::extra_small, ButtonShape::round}, fonts);
        widgets::Switch spec_switch;
        widgets::Label spec_label({"2025", layout.caption_type, layout.caption, false}, fonts);
        auto retheme = [&] { theme.fade_to((spec_switch.checked() ? color::spec_2025() : color::spec_2021()).build(options)); };
        const auto caption_style = tokens::text_style(tokens::type_style(layout.caption_type));

        const std::pair<ButtonStyle, const char*> styles[] = {
            {ButtonStyle::filled, "Filled"},     {ButtonStyle::tonal, "Tonal"}, {ButtonStyle::outlined, "Outlined"},
            {ButtonStyle::elevated, "Elevated"}, {ButtonStyle::text, "Text"},
        };
        widgets::WidgetSet controls;
        std::vector<Row> rows;
        for (const auto& [style, name] : styles) {
            Row row{std::make_unique<widgets::Label>(widgets::LabelOptions{name, layout.caption_type, layout.caption, false}, fonts), {}};
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"\u4fdd\u5b58", style, ButtonSize::small, ButtonShape::round}, fonts));
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"Send", style, ButtonSize::small, ButtonShape::square}, fonts));
            row.buttons.push_back(&controls.emplace<widgets::Button>(widgets::ButtonOptions{"\u7d9a\u3051\u308b", style, ButtonSize::medium, ButtonShape::round}, fonts));
            rows.push_back(std::move(row));
        }
        Row sizes{std::make_unique<widgets::Label>(widgets::LabelOptions{"Sizes", layout.caption_type, layout.caption, false}, fonts), {}};
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

        widgets::Label header({"\u30dc\u30bf\u30f3", layout.header_type, layout.text, false}, fonts);
        int clicks = 0;
        widgets::Label counter({"\u62bc\u3057\u305f\u56de\u6570: 0", layout.caption_type, layout.caption, false}, fonts);
        widgets::Label theme_caption({"Theme", layout.caption_type, layout.caption, false}, fonts);

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
            auto r = layout::row({examples::caption_cell(*row.caption, layout.label_width, caption_style.line_height, caption_style.line_height), buttons_row});
            r->cross_align(layout::CrossAlign::center);
            page_children.push_back(r);
        }
        std::vector<layout::NodePtr> theme_controls = chips.nodes();
        theme_controls.push_back(widgets::node(dark_toggle));
        theme_controls.push_back(widgets::node(spec_label));
        theme_controls.push_back(widgets::node(spec_switch));
        auto chips_row = layout::row(std::move(theme_controls));
        chips_row->spacing(chips.style().gap).cross_align(layout::CrossAlign::center);
        auto theme_row = layout::row(
            {examples::caption_cell(theme_caption, layout.label_width, chips.style().size, caption_style.line_height), chips_row});
        theme_row->cross_align(layout::CrossAlign::center);
        page_children.push_back(theme_row);
        auto page = layout::column(std::move(page_children));
        page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);

        color::ColorScheme scheme = theme.current();
        auto update = [&](const sdl3::Frame& frame) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
            scheme = theme.current();

            for (const auto& e : frame.pointer) {
                if (dark_toggle.handle(e)) {
                    options.dark = !options.dark;
                    retheme();
                }
                if (spec_switch.handle(e)) {
                    retheme();
                }
                if (const auto seed = chips.handle(e)) {
                    options.seed = *seed;
                    retheme();
                }
                for (auto& row : rows) {
                    for (auto& b : row.buttons) {
                        if (b->handle(e)) {
                            ++clicks;
                            counter.set_text("\u62bc\u3057\u305f\u56de\u6570: " + std::to_string(clicks));
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
            chips.paint(canvas, scheme);
            theme.step(frame.dt_seconds);
            controls.step(frame.dt_seconds);
            controls.paint(canvas, scheme);
            return !theme.settled() || controls.animating();
        };

        sdl3::run_app({"lamina-ui buttons", 1000, 760}, examples::window_canvas(), update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
