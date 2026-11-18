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
#include "lamina/motion/animated.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"

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

const std::vector<std::string> default_fonts{"C:/Windows/Fonts/NotoSansJP-VF.ttf"};

class Item {
public:
    virtual ~Item() = default;
    virtual double width() const = 0;
    virtual double height() const = 0;
    virtual void set_origin(render::Point p) = 0;
    virtual bool handle(const platform::PointerEvent& e) = 0;
    virtual void step(double dt) = 0;
    virtual bool animating() const = 0;
    virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
    std::function<void()> on_click;
};

template <typename W>
class Holder : public Item {
public:
    template <typename... Args>
    explicit Holder(Args&&... args) : widget(std::forward<Args>(args)...) {}
    double width() const override { return widget.width(); }
    double height() const override { return widget.height(); }
    void set_origin(render::Point p) override { widget.set_origin(p); }
    bool handle(const platform::PointerEvent& e) override { return widget.handle(e); }
    void step(double dt) override { widget.step(dt); }
    bool animating() const override { return widget.animating(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override { widget.paint(canvas, scheme); }
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
    double top = 0.0;
    double height = 0.0;
};

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
        text::FontStack fonts;
        for (int i = 1; i < argc; ++i) {
            fonts.add(text::Font::from_file(argv[i]));
        }
        if (fonts.empty()) {
            for (const auto& path : default_fonts) {
                fonts.add(text::Font::from_file(path));
            }
        }
        const icons::IconFont icon_font = icons::IconFont::from_directory(LAMINA_ICONS_DIR);
        auto icon = [&](Symbol s) { return icons::Icon{icon_font, s}; };

        color::SchemeOptions options = color::default_scheme_options;
        color::ColorScheme from = color::spec_2021().build(options);
        color::ColorScheme to = from;
        motion::Animated theme(1.0, tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::slow));
        std::size_t seed_index = 0;
        bool retheme_pending = false;

        const auto caption_style = text::text_style(tokens::type_style(layout.caption_type));
        const auto header_style = text::text_style(tokens::type_style(layout.header_type));
        std::string status = "\u2014";
        auto status_line = text::layout_line(fonts, status, caption_style);
        auto report = [&](std::string s) {
            status = std::move(s);
            status_line = text::layout_line(fonts, status, caption_style);
        };

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
        const auto header = text::layout_line(fonts, "\u90e8\u54c1", header_style);

        platform::Window window({"lamina-ui components", 1100, 860});
        platform::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            double y = layout.margin + header_style.line_height + layout.row_gap;
            for (auto& row : rows) {
                double x = layout.margin + layout.label_width;
                double row_height = 0.0;
                for (auto& item : row.items) {
                    row_height = std::max(row_height, item->height());
                }
                for (auto& item : row.items) {
                    item->set_origin({x, y + (row_height - item->height()) / 2.0});
                    x += item->width() + layout.column_gap;
                }
                row.top = y;
                row.height = row_height;
                y += row_height + layout.row_gap;
            }
            const double symbols_y = y;
            const double content_x = layout.margin + layout.label_width;
            const double per_line = std::max(1.0, std::floor((frame.width_dp - content_x - layout.margin) /
                                                             (layout.symbol_size + layout.symbol_gap)));
            const auto symbol_lines = static_cast<double>((icons::all_symbols.size() + static_cast<std::size_t>(per_line) - 1) /
                                                          static_cast<std::size_t>(per_line));
            const double chips_y = symbols_y + symbol_lines * (layout.symbol_size + layout.symbol_gap) + layout.row_gap;
            dark_toggle.set_origin({content_x + seeds.size() * (layout.chip + layout.chip_gap), chips_y - (dark_toggle.height() - layout.chip) / 2.0});
            const color::ColorScheme scheme = color::mix(from, to, theme.value());

            for (const auto& e : frame.pointer) {
                if (dark_toggle.handle(e)) {
                    options.dark = dark_toggle.checked();
                    retheme_pending = true;
                }
                if (e.action == platform::PointerAction::up) {
                    for (std::size_t i = 0; i < seeds.size(); ++i) {
                        const double cx = content_x + i * (layout.chip + layout.chip_gap) + layout.chip / 2.0;
                        const double cy = chips_y + layout.chip / 2.0;
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
            text::draw_line(canvas, fonts, header, {layout.margin, layout.margin}, scheme[layout.text]);
            text::draw_line(canvas, fonts, status_line, {frame.width_dp - layout.margin - status_line.width, layout.margin},
                            scheme[layout.caption]);
            animating = false;
            for (auto& row : rows) {
                const auto caption = text::layout_line(fonts, row.caption, caption_style);
                text::draw_line(canvas, fonts, caption, {layout.margin, row.top + (row.height - caption_style.line_height) / 2.0},
                                scheme[layout.caption]);
                for (auto& item : row.items) {
                    item->step(frame.dt_seconds);
                    item->paint(canvas, scheme);
                    animating = animating || item->animating();
                }
            }
            const auto symbols_caption = text::layout_line(fonts, "Symbols", caption_style);
            text::draw_line(canvas, fonts, symbols_caption, {layout.margin, symbols_y + (layout.symbol_size - caption_style.line_height) / 2.0},
                            scheme[layout.caption]);
            for (std::size_t i = 0; i < icons::all_symbols.size(); ++i) {
                const auto col = static_cast<double>(i % static_cast<std::size_t>(per_line));
                const auto line = static_cast<double>(i / static_cast<std::size_t>(per_line));
                icon_font.draw(canvas, icons::all_symbols[i],
                               {content_x + col * (layout.symbol_size + layout.symbol_gap), symbols_y + line * (layout.symbol_size + layout.symbol_gap)},
                               layout.symbol_size, scheme[layout.caption]);
            }
            const auto theme_caption = text::layout_line(fonts, "Theme", caption_style);
            text::draw_line(canvas, fonts, theme_caption, {layout.margin, chips_y + (layout.chip - caption_style.line_height) / 2.0},
                            scheme[layout.caption]);
            for (std::size_t i = 0; i < seeds.size(); ++i) {
                const render::Rect c{content_x + i * (layout.chip + layout.chip_gap), chips_y, layout.chip, layout.chip};
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
