#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/canvas.hpp"
#include "common/caption_cell.hpp"
#include "common/options.hpp"
#include "common/theme.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"

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
};

class Item {
public:
    virtual ~Item() = default;
    virtual bool handle(const input::PointerEvent& e) = 0;
    virtual bool handle(const input::KeyEvent& e) = 0;
    virtual void join(widgets::FocusManager& focus) = 0;
    virtual layout::NodePtr node() = 0;
    std::function<void()> on_click;
};

template <typename W>
class Holder : public Item {
public:
    explicit Holder(W& w) : widget(w) {}
    bool handle(const input::PointerEvent& e) override { return widget.handle(e); }
    bool handle(const input::KeyEvent& e) override { return widget.handle(e); }
    void join(widgets::FocusManager& focus) override { focus.add(widget); }
    layout::NodePtr node() override { return widgets::node(widget); }
    W& widget;
};

template <typename W, typename... Args>
Holder<W>* add(widgets::WidgetSet& controls, std::vector<std::unique_ptr<Item>>& items, Args&&... args) {
    auto holder = std::make_unique<Holder<W>>(controls.emplace<W>(std::forward<Args>(args)...));
    Holder<W>* raw = holder.get();
    items.push_back(std::move(holder));
    return raw;
}

struct Row {
    std::string caption;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<widgets::Label> label;
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
        const bool want_title_bar = examples::take_flag(argc, argv, examples::title_bar_flag);
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        auto icon = [&](Symbol s) { return icons::Icon{icon_font, s}; };

        color::SchemeOptions options = color::default_scheme_options;
        examples::ThemeFade theme(color::spec_2021().build(options));
        examples::SeedChips chips;
        bool retheme_pending = false;

        const auto caption_style = tokens::text_style(tokens::type_style(layout.caption_type));
        widgets::Label status({"\u2014", layout.caption_type, layout.caption, false}, fonts);
        auto report = [&](std::string s) { status.set_text(std::move(s)); };

        widgets::WidgetSet controls;
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
            auto* b = add<widgets::Button>(controls, buttons.items,
                                           widgets::ButtonOptions{label, style, ButtonSize::small, ButtonShape::round, icon(content.first)},
                                           fonts);
            b->on_click = [&report, label] { report("Button: " + label); };
        }
        auto* medium = add<widgets::Button>(
            controls, buttons.items,
            widgets::ButtonOptions{"\u4fdd\u5b58", ButtonStyle::filled, ButtonSize::medium, ButtonShape::square, icon(Symbol::download)}, fonts);
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
            auto* b = add<widgets::IconButton>(controls, icon_buttons.items, widgets::IconButtonOptions{icon(Symbol::settings), style});
            b->on_click = [&report, n] { report("Icon button: " + n); };
        }
        const Symbol toggle_symbols[] = {Symbol::favorite, Symbol::star, Symbol::bookmark, Symbol::notifications};
        for (std::size_t i = 0; i < std::size(icon_styles); ++i) {
            const std::string n = icon_styles[i].second;
            auto* b = add<widgets::IconButton>(controls, icon_buttons.items,
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
            auto* b = add<widgets::IconButton>(controls, sizes.items,
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
            auto* f = add<widgets::Fab>(controls, fabs.items, widgets::FabOptions{icon(symbol), size});
            f->on_click = [&report] { report("FAB"); };
        }
        auto* secondary = add<widgets::Fab>(controls, fabs.items, widgets::FabOptions{icon(Symbol::search), tokens::FabSize::baseline,
                                                                          tokens::FabStyle::secondary_container});
        secondary->on_click = [&report] { report("FAB: secondary"); };
        rows.push_back(std::move(fabs));

        Row switches{"Switch"};
        auto* plain_switch = add<widgets::Switch>(controls, switches.items);
        plain_switch->on_click = [&report, &w = plain_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        auto* icon_switch = add<widgets::Switch>(controls, switches.items, widgets::SwitchOptions{true, icon(Symbol::check), std::nullopt});
        icon_switch->on_click = [&report, &w = icon_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        auto* both_switch = add<widgets::Switch>(controls, switches.items, widgets::SwitchOptions{false, icon(Symbol::check), icon(Symbol::close)});
        both_switch->on_click = [&report, &w = both_switch->widget] { report(w.checked() ? "Switch: on" : "Switch: off"); };
        rows.push_back(std::move(switches));

        Row checkboxes{"Checkbox"};
        for (auto state : {tokens::CheckboxState::off, tokens::CheckboxState::on, tokens::CheckboxState::indeterminate}) {
            auto* c = add<widgets::Checkbox>(controls, checkboxes.items, widgets::CheckboxOptions{state});
            c->on_click = [&report, &w = c->widget] { report(std::string("Checkbox: ") + checkbox_name(w.state())); };
        }
        rows.push_back(std::move(checkboxes));

        Row radios{"Radio button"};
        std::vector<widgets::RadioButton*> group;
        for (int i = 0; i < 3; ++i) {
            auto* r = add<widgets::RadioButton>(controls, radios.items, widgets::RadioButtonOptions{i == 0});
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
        controls.add(dark_toggle);
        widgets::Label header({"\u90e8\u54c1", layout.header_type, layout.text, false}, fonts);
        widgets::Label symbols_caption({"Symbols", layout.caption_type, layout.caption, false}, fonts);
        widgets::Label theme_caption({"Theme", layout.caption_type, layout.caption, false}, fonts);

        auto header_row = layout::row({widgets::node(header), widgets::node(status)});
        header_row->fill_main(true).main_align(layout::MainAlign::space_between);
        header_row->self_align = layout::CrossAlign::stretch;
        std::vector<layout::NodePtr> page_children{header_row};
        for (auto& row : rows) {
            row.label = std::make_unique<widgets::Label>(widgets::LabelOptions{row.caption, layout.caption_type, layout.caption, false}, fonts);
            std::vector<layout::NodePtr> nodes;
            for (auto& item : row.items) {
                nodes.push_back(item->node());
            }
            auto items_row = layout::row(std::move(nodes));
            items_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
            auto r = layout::row({examples::caption_cell(*row.label, layout.label_width, caption_style.line_height, caption_style.line_height), items_row});
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
            layout::row({examples::caption_cell(symbols_caption, layout.label_width, layout.symbol_size, caption_style.line_height), symbols}));

        std::vector<layout::NodePtr> theme_controls = chips.nodes();
        theme_controls.push_back(layout::fixed({dark_toggle.width(), chips.style().size}, [&dark_toggle](const render::Rect& r) {
            dark_toggle.set_origin({r.x, r.y + (r.height - dark_toggle.height()) / 2.0});
        }));
        auto chips_row = layout::row(std::move(theme_controls));
        chips_row->spacing(chips.style().gap).cross_align(layout::CrossAlign::center);
        auto theme_row = layout::row(
            {examples::caption_cell(theme_caption, layout.label_width, chips.style().size, caption_style.line_height), chips_row});
        theme_row->cross_align(layout::CrossAlign::center);
        page_children.push_back(theme_row);
        auto page = layout::column(std::move(page_children));
        page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);

        const char* title = "lamina-ui components";
        sdl3::Window window({title, 1100, examples::window_height_dp(860, want_title_bar), want_title_bar}, examples::window_canvas());
        widgets::IconButton bar_search({icon(Symbol::search), IconButtonStyle::standard, IconButtonSize::extra_small});
        widgets::TitleBar title_bar({title, icon_font, layout::row({widgets::centered_node(bar_search)})}, fonts);
        layout::NodePtr root = page;
        if (window.client_title_bar()) {
            controls.add(title_bar);
            controls.add(bar_search);
            page->self_align = layout::CrossAlign::stretch;
            auto framed = layout::column({widgets::node(title_bar), layout::weighted(page, 1.0)});
            framed->fill_cross(true);
            root = framed;
        }

        widgets::FocusManager focus;
        if (window.client_title_bar()) {
            focus.add(bar_search);
        }
        for (auto& row : rows) {
            for (auto& item : row.items) {
                item->join(focus);
            }
        }
        focus.add(dark_toggle);

        color::ColorScheme scheme = theme.current();
        auto update = [&](const sdl3::Frame& frame) {
            title_bar.set_maximized(frame.maximized);
            title_bar.set_active(frame.active);
            layout::run(*root, {0.0, 0.0, frame.width_dp, frame.height_dp});
            window.set_caption_regions(title_bar.regions(std::array{bar_search.bounds()}));
            scheme = theme.current();

            for (const auto& e : frame.pointer) {
                if (window.client_title_bar()) {
                    if (bar_search.handle(e)) {
                        report("Title bar: search");
                    }
                    switch (title_bar.handle(e)) {
                    case widgets::TitleBarAction::minimize:
                        window.minimize();
                        break;
                    case widgets::TitleBarAction::toggle_maximize:
                        window.toggle_maximize();
                        break;
                    case widgets::TitleBarAction::close:
                        window.close();
                        break;
                    case widgets::TitleBarAction::none:
                        break;
                    }
                }
                if (dark_toggle.handle(e)) {
                    options.dark = dark_toggle.checked();
                    retheme_pending = true;
                }
                if (const auto seed = chips.handle(e)) {
                    options.seed = *seed;
                    retheme_pending = true;
                }
                for (auto& row : rows) {
                    for (auto& item : row.items) {
                        if (item->handle(e) && item->on_click) {
                            item->on_click();
                        }
                    }
                }
            }
            for (const auto& k : frame.keys) {
                if (focus.handle(k)) {
                    continue;
                }
                if (window.client_title_bar() && bar_search.handle(k)) {
                    report("Title bar: search");
                }
                if (dark_toggle.handle(k)) {
                    options.dark = dark_toggle.checked();
                    retheme_pending = true;
                }
                for (auto& row : rows) {
                    for (auto& item : row.items) {
                        if (item->handle(k) && item->on_click) {
                            item->on_click();
                        }
                    }
                }
            }
            if (retheme_pending) {
                retheme_pending = false;
                theme.fade_to(color::spec_2021().build(options));
            }
            return scheme[color::Role::surface];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            header.paint(canvas, scheme);
            status.paint(canvas, scheme);
            for (auto& row : rows) {
                row.label->paint(canvas, scheme);
            }
            symbols_caption.paint(canvas, scheme);
            for (std::size_t i = 0; i < icons::all_symbols.size(); ++i) {
                const auto col = static_cast<double>(i % per_line);
                const auto line = static_cast<double>(i / per_line);
                icon_font.draw(canvas, icons::all_symbols[i], {symbols_at.x + col * symbol_pitch, symbols_at.y + line * symbol_pitch},
                               layout.symbol_size, scheme[layout.caption]);
            }
            theme_caption.paint(canvas, scheme);
            chips.paint(canvas, scheme);
            theme.step(frame.dt_seconds);
            controls.step(frame.dt_seconds);
            controls.paint(canvas, scheme);
            return !theme.settled() || controls.animating();
        };

        sdl3::run_app(window, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
