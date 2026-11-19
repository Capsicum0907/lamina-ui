#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "lamina_sdl3/window.hpp"

using namespace lamina;
using icons::Symbol;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using tokens::IconButtonSize;
using tokens::IconButtonStyle;
using tokens::IconButtonWidth;

namespace {

struct Layout {
    double margin = 32.0;
    double row_gap = 20.0;
    double column_gap = 16.0;
    double label_width = 140.0;
    color::Role text = color::Role::on_surface;
    color::Role caption = color::Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole caption_type = tokens::TypeRole::label_large;
    double symbol_size = 24.0;
    double symbol_gap = 12.0;
    double chip = 32.0;
    double chip_gap = 12.0;
    color::Role chip_ring = color::Role::on_surface;
    double chip_ring_width = 2.0;
};

const std::vector<color::Argb> seeds{0xff6750a4, 0xff006a6a, 0xffb3261e, 0xff7d5260, 0xff386a20};

class Item {
public:
    virtual ~Item() = default;
    virtual bool handle(const input::PointerEvent& e) = 0;
    virtual void step(double dt) = 0;
    virtual bool animating() const = 0;
    virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
    virtual layout::NodePtr node() = 0;
    std::function<void()> on_click;
};

template <typename W>
class Holder : public Item {
public:
    template <typename... Args>
    explicit Holder(Args&&... args) : widget(std::forward<Args>(args)...) {}
    bool handle(const input::PointerEvent& e) override { return widget.handle(e); }
    void step(double dt) override { widget.step(dt); }
    bool animating() const override { return widget.animating(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override { widget.paint(canvas, scheme); }
    layout::NodePtr node() override { return widgets::node(widget); }
    W widget;
};

template <typename W, typename... Args>
Holder<W>* add(std::vector<std::unique_ptr<Item>>& items, Args&&... args) {
    auto holder = std::make_unique<Holder<W>>(std::forward<Args>(args)...);
    Holder<W>* raw = holder.get();
    items.push_back(std::move(holder));
    return raw;
}

struct Row {
    std::string caption;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<widgets::Label> label;
};

layout::NodePtr caption_cell(widgets::Label& caption, double width, double height, double line_height) {
    caption.measure({});
    return layout::fixed({width, height}, [&caption, line_height](const render::Rect& r) {
        caption.set_origin({r.x, r.y + (r.height - line_height) / 2.0});
    });
}

const char* checkbox_name(tokens::CheckboxState s) {
    switch (s) {
    case tokens::CheckboxState::on:
        return "on";
    case tokens::CheckboxState::indeterminate:
        return "indeterminate";
    case tokens::CheckboxState::off:
        break;
    }
    return "off";
}

}

int main(int argc, char** argv) {
    try {
        const Layout layout;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        auto icon = [&](Symbol s) { return icons::Icon{icon_font, s}; };

        color::SchemeOptions options = color::default_scheme_options;
        color::ColorScheme from = color::spec_2021().build(options);
        color::ColorScheme to = from;
        motion::Animated theme(1.0, tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::slow));
        std::size_t seed_index = 0;
        bool retheme_pending = false;

        const auto caption_style = text::text_style(tokens::type_style(layout.caption_type));
        widgets::Label status("\u2014", layout.caption_type, layout.caption, fonts, false);
        auto report = [&](std::string s) { status.set_text(std::move(s)); };

        std::vector<Row> rows;

        Row buttons{"Buttons"};
        const std::pair<ButtonStyle, std::pair<Symbol, const char*>> button_list[] = {
            {ButtonStyle::filled, {Symbol::add, "\u8ffd\u52a0"}},
            {ButtonStyle::tonal, {Symbol::send, "Send"}},
            {ButtonStyle::outlined, {Symbol::edit, "\u7de8\u96c6"}},
            {ButtonStyle::elevated, {Symbol::favorite, "Like"}},
            {ButtonStyle::text, {Symbol::share, "\u5171\u6709"}},
        };
        for (const auto& [style, content] : button_list) {
            const std::string label = content.second;
            auto* b = add<widgets::Button>(buttons.items,
                                           widgets::ButtonOptions{label, style, ButtonSize::small, ButtonShape::round, icon(content.first)},
                                           fonts);
            b->on_click = [&report, label] { report("Button: " + label); };
        }
        auto* medium = add<widgets::Button>(
            buttons.items, widgets::ButtonOptions{"\u4fdd\u5b58", ButtonStyle::filled, ButtonSize::medium, ButtonShape::square, icon(Symbol::download)},
            fonts);
        medium->on_click = [&report] { report("Button: medium"); };
        rows.push_back(std::move(buttons));

        const std::pair<IconButtonStyle, const char*> icon_styles[] = {
            {IconButtonStyle::filled, "filled"},
            {IconButtonStyle::tonal, "tonal"},
            {IconButtonStyle::outlined, "outlined"},
            {IconButtonStyle::standard, "standard"},
        };
        Row icon_buttons{"Icon buttons"};
        for (const auto& [style, name] : icon_styles) {
            const std::string n = name;
            auto* b = add<widgets::IconButton>(icon_buttons.items, widgets::IconButtonOptions{icon(Symbol::settings), style});
            b->on_click = [&report, n] { report("Icon button: " + n); };
        }
        const Symbol toggle_symbols[] = {Symbol::favorite, Symbol::star, Symbol::bookmark, Symbol::notifications};
        for (std::size_t i = 0; i < std::size(icon_styles); ++i) {
            const std::string n = icon_styles[i].second;
            auto* b = add<widgets::IconButton>(icon_buttons.items,
                                               widgets::IconButtonOptions{icon(toggle_symbols[i]), icon_styles[i].first,
                                                                          IconButtonSize::small, ButtonShape::round,
                                                                          IconButtonWidth::uniform, true});
            auto& w = b->widget;
            b->on_click = [&report, &w, n] { report("Toggle " + n + (w.checked() ? ": on" : ": off")); };
        }
        rows.push_back(std::move(icon_buttons));

        Row sizes{"Sizes"};
        const IconButtonSize size_list[] = {IconButtonSize::extra_small, IconButtonSize::small, IconButtonSize::medium,
                                            IconButtonSize::large, IconButtonSize::extra_large};
        for (std::size_t i = 0; i < std::size(size_list); ++i) {
            const auto shape = i % 2 == 0 ? ButtonShape::round : ButtonShape::square;
            const auto width = i == 1 ? IconButtonWidth::wide : IconButtonWidth::uniform;
            auto* b = add<widgets::IconButton>(sizes.items,
                                               widgets::IconButtonOptions{icon(Symbol::palette), IconButtonStyle::tonal, size_list[i], shape, width});
            b->on_click = [&report, i] { report("Size " + std::to_string(i)); };
        }
        rows.push_back(std::move(sizes));

        Row fabs{"FAB"};
        const std::pair<tokens::FabSize, Symbol> fab_list[] = {
            {tokens::FabSize::baseline, Symbol::edit},
            {tokens::FabSize::medium, Symbol::add},
            {tokens::FabSize::large, Symbol::mail},
        };
        for (const auto& [size, symbol] : fab_list) {
            auto* f = add<widgets::Fab>(fabs.items, widgets::FabOptions{icon(symbol), size});
            f->on_click = [&report] { report("FAB"); };
        }
        auto* secondary = add<widgets::Fab>(fabs.items, widgets::FabOptions{icon(Symbol::search), tokens::FabSize::baseline,
                                                                          tokens::FabStyle::secondary_container});
        secondary->on_click = [&report] { report("FAB: secondary"); };
        rows.push_back(std::move(fabs));

        Row switches{"Switch"};
        auto* plain_switch = add<widgets::Switch>(switches.items);
        plain_switch->on_click = [&report, &w = plain_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        auto* icon_switch = add<widgets::Switch>(switches.items, widgets::SwitchOptions{true, icon(Symbol::check), std::nullopt});
        icon_switch->on_click = [&report, &w = icon_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        auto* both_switch = add<widgets::Switch>(switches.items, widgets::SwitchOptions{false, icon(Symbol::check), icon(Symbol::close)});
        both_switch->on_click = [&report, &w = both_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        rows.push_back(std::move(switches));

        Row checkboxes{"Checkbox"};
        for (auto state : {tokens::CheckboxState::off, tokens::CheckboxState::on, tokens::CheckboxState::indeterminate}) {
            auto* c = add<widgets::Checkbox>(checkboxes.items, widgets::CheckboxOptions{state});
            c->on_click = [&report, &w = c->widget] { report(std::string("Checkbox: ") + checkbox_name(w.state())); };
        }
        rows.push_back(std::move(checkboxes));

        Row radios{"Radio button"};
        std::vector<widgets::RadioButton*> group;
        for (int i = 0; i < 3; ++i) {
            auto* r = add<widgets::RadioButton>(radios.items, widgets::RadioButtonOptions{i == 0});
            group.push_back(&r->widget);
            r->on_click = [&report, &group, i] {
                for (int k = 0; k < static_cast<int>(group.size()); ++k) {
                    group[k]->set_selected(k == i);
                }
                report("Radio: " + std::to_string(i + 1));
            };
        }
        rows.push_back(std::move(radios));

        widgets::IconButton dark_toggle({icon(options.dark ? Symbol::light_mode : Symbol::dark_mode), IconButtonStyle::outlined,
                                         IconButtonSize::small, ButtonShape::round, IconButtonWidth::uniform, true});
        widgets::Label header("\u90e8\u54c1", layout.header_type, layout.text, fonts, false);
        widgets::Label symbols_caption("Symbols", layout.caption_type, layout.caption, fonts, false);
        widgets::Label theme_caption("Theme", layout.caption_type, layout.caption, fonts, false);

        auto header_row = layout::row({widgets::node(header), widgets::node(status)});
        header_row->fill_main(true).main_align(layout::MainAlign::space_between);
        header_row->self_align = layout::CrossAlign::stretch;
        std::vector<layout::NodePtr> page_children{header_row};
        for (auto& row : rows) {
            row.label = std::make_unique<widgets::Label>(row.caption, layout.caption_type, layout.caption, fonts, false);
            std::vector<layout::NodePtr> nodes;
            for (auto& item : row.items) {
                nodes.push_back(item->node());
            }
            auto items_row = layout::row(std::move(nodes));
            items_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
            auto r = layout::row({caption_cell(*row.label, layout.label_width, caption_style.line_height, caption_style.line_height), items_row});
            r->cross_align(layout::CrossAlign::center);
            page_children.push_back(r);
        }

        const double symbol_pitch = layout.symbol_size + layout.symbol_gap;
        std::size_t per_line = 1;
        render::Point symbols_at{0.0, 0.0};
        auto symbols = std::make_shared<layout::Leaf>(
            [&per_line, symbol_pitch](const layout::Constraints& c) {
                per_line = static_cast<std::size_t>(std::max(1.0, std::floor(c.max_width / symbol_pitch)));
                const std::size_t lines = (icons::all_symbols.size() + per_line - 1) / per_line;
                return layout::Size{static_cast<double>(per_line) * symbol_pitch, static_cast<double>(lines) * symbol_pitch};
            },
            [&symbols_at](const render::Rect& r) { symbols_at = {r.x, r.y}; });
        page_children.push_back(
            layout::row({caption_cell(symbols_caption, layout.label_width, layout.symbol_size, caption_style.line_height), symbols}));

        std::vector<render::Rect> chip_rects(seeds.size());
        std::vector<layout::NodePtr> chips;
        for (std::size_t i = 0; i < seeds.size(); ++i) {
            chips.push_back(layout::fixed({layout.chip, layout.chip}, [&chip_rects, i](const render::Rect& r) { chip_rects[i] = r; }));
        }
        chips.push_back(layout::fixed({dark_toggle.width(), layout.chip}, [&dark_toggle](const render::Rect& r) {
            dark_toggle.set_origin({r.x, r.y + (r.height - dark_toggle.height()) / 2.0});
        }));
        auto chips_row = layout::row(std::move(chips));
        chips_row->spacing(layout.chip_gap).cross_align(layout::CrossAlign::center);
        auto theme_row = layout::row({caption_cell(theme_caption, layout.label_width, layout.chip, caption_style.line_height), chips_row});
        theme_row->cross_align(layout::CrossAlign::center);
        page_children.push_back(theme_row);
        auto page = layout::column(std::move(page_children));
        page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);

        sdl3::Window window({"lamina-ui components", 1100, 860});
        sdl3::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
            const color::ColorScheme scheme = color::mix(from, to, theme.value());

            for (const auto& e : frame.pointer) {
                if (dark_toggle.handle(e)) {
                    options.dark = dark_toggle.checked();
                    retheme_pending = true;
                }
                if (e.action == input::PointerAction::up) {
                    for (std::size_t i = 0; i < seeds.size(); ++i) {
                        const double cx = chip_rects[i].x + chip_rects[i].width / 2.0;
                        const double cy = chip_rects[i].y + chip_rects[i].height / 2.0;
                        if (std::hypot(e.x_dp - cx, e.y_dp - cy) <= layout.chip / 2.0 && i != seed_index) {
                            seed_index = i;
                            options.seed = seeds[i];
                            retheme_pending = true;
                        }
                    }
                }
                for (auto& row : rows) {
                    for (auto& item : row.items) {
                        if (item->handle(e) && item->on_click) {
                            item->on_click();
                        }
                    }
                }
            }
            if (retheme_pending) {
                retheme_pending = false;
                from = scheme;
                to = color::spec_2021().build(options);
                theme.snap_to(0.0);
                theme.animate_to(1.0);
            }

            auto& canvas = window.canvas();
            canvas.clear(scheme[color::Role::surface]);
            canvas.save();
            canvas.scale(frame.scale);
            header.paint(canvas, scheme);
            status.paint(canvas, scheme);
            animating = false;
            for (auto& row : rows) {
                row.label->paint(canvas, scheme);
                for (auto& item : row.items) {
                    item->step(frame.dt_seconds);
                    item->paint(canvas, scheme);
                    animating = animating || item->animating();
                }
            }
            symbols_caption.paint(canvas, scheme);
            for (std::size_t i = 0; i < icons::all_symbols.size(); ++i) {
                const auto col = static_cast<double>(i % per_line);
                const auto line = static_cast<double>(i / per_line);
                icon_font.draw(canvas, icons::all_symbols[i], {symbols_at.x + col * symbol_pitch, symbols_at.y + line * symbol_pitch},
                               layout.symbol_size, scheme[layout.caption]);
            }
            theme_caption.paint(canvas, scheme);
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
