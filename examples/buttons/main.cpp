#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/button.hpp"

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

const std::vector<std::string> default_fonts{"C:/Windows/Fonts/NotoSansJP-VF.ttf"};

struct Row {
    std::string caption;
    std::vector<std::unique_ptr<widgets::Button>> buttons;
};

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        text::FontStack fonts;
        for (int i = 1; i < argc; ++i) {
            fonts.add(text::Font::from_file(argv[i]));
        }
        if (fonts.empty()) {
            for (const auto& path : default_fonts) {
                fonts.add(text::Font::from_file(path));
            }
        }
        color::SchemeOptions options = color::default_scheme_options;
        color::ColorScheme from = color::spec_2021().build(options);
        color::ColorScheme to = from;
        motion::Animated theme(1.0, tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::slow));
        std::size_t seed_index = 0;
        widgets::Button dark_toggle({"ダーク", ButtonStyle::outlined, ButtonSize::extra_small, ButtonShape::round}, fonts);
        auto retheme = [&](const color::ColorScheme& current) {
            from = current;
            to = color::spec_2021().build(options);
            theme.snap_to(0.0);
            theme.animate_to(1.0);
        };
        const auto caption_style = text::text_style(tokens::type_style(layout.caption_type));
        const auto header_style = text::text_style(tokens::type_style(layout.header_type));

        const std::pair<ButtonStyle, const char*> styles[] = {
            {ButtonStyle::filled, "Filled"},     {ButtonStyle::tonal, "Tonal"}, {ButtonStyle::outlined, "Outlined"},
            {ButtonStyle::elevated, "Elevated"}, {ButtonStyle::text, "Text"},
        };
        std::vector<Row> rows;
        for (const auto& [style, name] : styles) {
            Row row{name, {}};
            row.buttons.push_back(std::make_unique<widgets::Button>(widgets::ButtonOptions{"\u4fdd\u5b58", style, ButtonSize::small, ButtonShape::round}, fonts));
            row.buttons.push_back(std::make_unique<widgets::Button>(widgets::ButtonOptions{"Send", style, ButtonSize::small, ButtonShape::square}, fonts));
            row.buttons.push_back(std::make_unique<widgets::Button>(widgets::ButtonOptions{"\u7d9a\u3051\u308b", style, ButtonSize::medium, ButtonShape::round}, fonts));
            rows.push_back(std::move(row));
        }
        Row sizes{"Sizes", {}};
        const std::pair<ButtonSize, const char*> size_list[] = {
            {ButtonSize::extra_small, "XS"}, {ButtonSize::small, "S"}, {ButtonSize::medium, "M"},
            {ButtonSize::large, "L"},        {ButtonSize::extra_large, "XL"},
        };
        for (const auto& [size, name] : size_list) {
            sizes.buttons.push_back(std::make_unique<widgets::Button>(widgets::ButtonOptions{name, ButtonStyle::filled, size, ButtonShape::round}, fonts));
        }
        rows.push_back(std::move(sizes));

        const auto header = text::layout_line(fonts, "\u30dc\u30bf\u30f3", header_style);
        int clicks = 0;
        auto counter = text::layout_line(fonts, "\u62bc\u3057\u305f\u56de\u6570: 0", caption_style);

        platform::Window window({"lamina-ui buttons", 1000, 760});
        platform::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            double y = layout.margin + header_style.line_height + layout.row_gap;
            for (auto& row : rows) {
                double x = layout.margin + layout.label_width;
                double row_height = 0.0;
                for (auto& b : row.buttons) {
                    row_height = std::max(row_height, b->height());
                }
                for (auto& b : row.buttons) {
                    b->set_origin({x, y + (row_height - b->height()) / 2.0});
                    x += b->width() + layout.column_gap;
                }
                y += row_height + layout.row_gap;
            }
            const double chips_y = y;
            const double chips_x = layout.margin + layout.label_width;
            dark_toggle.set_origin({chips_x + seeds.size() * (layout.chip + layout.chip_gap), chips_y});
            const color::ColorScheme scheme = color::mix(from, to, theme.value());

            for (const auto& e : frame.pointer) {
                if (dark_toggle.handle(e)) {
                    options.dark = !options.dark;
                    retheme(scheme);
                }
                if (e.action == platform::PointerAction::up) {
                    for (std::size_t i = 0; i < seeds.size(); ++i) {
                        const double cx = chips_x + i * (layout.chip + layout.chip_gap) + layout.chip / 2.0;
                        const double cy = chips_y + layout.chip / 2.0;
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
                            counter = text::layout_line(fonts, "\u62bc\u3057\u305f\u56de\u6570: " + std::to_string(clicks), caption_style);
                        }
                    }
                }
            }

            auto& canvas = window.canvas();
            canvas.clear(scheme[color::Role::surface]);
            canvas.save();
            canvas.scale(frame.scale);
            text::draw_line(canvas, fonts, header, {layout.margin, layout.margin}, scheme[layout.text]);
            text::draw_line(canvas, fonts, counter, {frame.width_dp - layout.margin - counter.width, layout.margin}, scheme[layout.caption]);
            animating = false;
            for (auto& row : rows) {
                const auto& first = row.buttons.front();
                const auto caption = text::layout_line(fonts, row.caption, caption_style);
                const double cy = first->bounds().y + first->height() / 2.0;
                text::draw_line(canvas, fonts, caption, {layout.margin, cy - caption_style.line_height / 2.0}, scheme[layout.caption]);
                for (auto& b : row.buttons) {
                    b->step(frame.dt_seconds);
                    b->paint(canvas, scheme);
                    animating = animating || b->animating();
                }
            }
            const auto theme_caption = text::layout_line(fonts, "Theme", caption_style);
            text::draw_line(canvas, fonts, theme_caption, {layout.margin, chips_y + (layout.chip - caption_style.line_height) / 2.0}, scheme[layout.caption]);
            for (std::size_t i = 0; i < seeds.size(); ++i) {
                const render::Rect c{chips_x + i * (layout.chip + layout.chip_gap), chips_y, layout.chip, layout.chip};
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
            dark_toggle.step(frame.dt_seconds);
            dark_toggle.paint(canvas, scheme);
            animating = animating || !theme.settled() || dark_toggle.animating();
            canvas.restore();
            window.present();
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
