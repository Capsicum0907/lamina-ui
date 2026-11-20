#include "components/gallery.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/caption_cell.hpp"
#include "common/theme.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/fab.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/title_bar.hpp"
#include "lamina/widgets/tooltip.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

using icons::Symbol;
using tokens::ButtonShape;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using tokens::IconButtonSize;
using tokens::IconButtonStyle;
using tokens::IconButtonWidth;
using tokens::TypeRole;
namespace res = examples::res;

struct Layout {
    double margin = 24.0;
    double row_gap = 16.0;
    double column_gap = 16.0;
    double column_space = 48.0;
    double label_width = 120.0;
    double slider_width = 240.0;
    double tabs_width = 380.0;
    double field_gap = 8.0;
    double type_gap = 24.0;
    color::Role text = color::Role::on_surface;
    color::Role caption = color::Role::on_surface_variant;
    TypeRole header_type = TypeRole::headline_small_emphasized;
    TypeRole caption_type = TypeRole::label_large;
    double symbol_size = 24.0;
    double symbol_gap = 12.0;
    int dialog_paragraphs = 10;
};

struct TypeSample {
    TypeRole role;
    const char* name;
};

constexpr std::array<TypeSample, 6> type_samples{{
    {TypeRole::display_small, "Display S"},
    {TypeRole::headline_small, "Headline S"},
    {TypeRole::title_large, "Title L"},
    {TypeRole::title_medium, "Title M"},
    {TypeRole::body_large, "Body L"},
    {TypeRole::label_large, "Label L"},
}};

struct Row {
    std::unique_ptr<widgets::Label> caption;
    layout::NodePtr content;
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

struct Gallery::Impl {
    Impl(const text::FontStack& f, const icons::IconFont& i, GalleryHost& h);

    icons::Icon icon(Symbol s) const { return {icon_font, s}; }
    void report(std::string s) { status.set_text(std::move(s)); }
    template <typename W, typename... Args>
    W& control(std::vector<layout::NodePtr>& row, Args&&... args);
    template <typename W, typename OnResult, typename... Args>
    W& control_with(std::vector<layout::NodePtr>& row, OnResult on_result, Args&&... args);
    layout::NodePtr captioned(const char* caption, layout::NodePtr content);
    layout::NodePtr items(std::vector<layout::NodePtr> nodes);
    void build_left(std::vector<layout::NodePtr>& column);
    void build_right(std::vector<layout::NodePtr>& column);
    void build_dialog();
    void add_tooltip(widgets::Tooltip tooltip);
    void retheme();
    void open_menu();
    void open_dialog();

    const Layout layout;
    text::FontStack fonts;
    icons::IconFont icon_font;
    GalleryHost& host;
    color::SchemeOptions options = color::default_scheme_options;
    ThemeFade theme;
    color::ColorScheme scheme;
    SeedChips chips;
    widgets::Label header;
    widgets::Label status;
    widgets::WidgetSet controls;
    widgets::WidgetSet chrome;
    widgets::FocusManager focus;
    widgets::OverlayStack overlay;
    std::vector<Row> rows;
    std::vector<std::unique_ptr<widgets::Tooltip>> tooltips;
    widgets::Switch* enabled_switch = nullptr;
    widgets::IconButton* dark_toggle = nullptr;
    widgets::Button* menu_button = nullptr;
    widgets::Button* dialog_button = nullptr;
    std::unique_ptr<widgets::Menu> menu;
    widgets::WidgetSet dialog_widgets;
    widgets::FocusManager dialog_focus;
    std::unique_ptr<widgets::Dialog> dialog;
    std::unique_ptr<widgets::TitleBar> title_bar;
    std::vector<widgets::RadioButton*> radios;
    std::map<std::string, std::function<void(bool)>, std::less<>> enablers;
    std::size_t symbols_per_line = 1;
    render::Point symbols_at{0.0, 0.0};
    layout::NodePtr root;
    input::InputSource source = input::InputSource::pointer;
    layout::Size window{static_cast<double>(window_width_dp), static_cast<double>(window_height_dp)};
};

Gallery::Impl::Impl(const text::FontStack& f, const icons::IconFont& i, GalleryHost& h)
    : fonts(f),
      icon_font(i),
      host(h),
      theme(color::spec_2021().build(options)),
      scheme(theme.current()),
      header({"部品", layout.header_type, layout.text, false}, f),
      status({"—", layout.caption_type, layout.caption, false}, f) {
    auto& switch_label = chrome.emplace<widgets::Label>(widgets::LabelOptions{"Enabled", layout.caption_type, layout.caption, false}, fonts);
    enabled_switch = &chrome.emplace_with<widgets::Switch>(
        [this](widgets::Switch& s, bool clicked) {
            if (clicked) {
                controls.set_enabled(s.checked());
                report(s.checked() ? "Enabled: on" : "Enabled: off");
            }
        },
        widgets::SwitchOptions{.checked = true, .checked_icon = icon(res::icons::check), .accessible_name = "Enabled"});
    focus.add(*enabled_switch);

    auto toggle_row = layout::row({widgets::node(switch_label), widgets::node(*enabled_switch)});
    toggle_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
    auto end_row = layout::row({widgets::node(status), toggle_row});
    end_row->spacing(layout.column_space).cross_align(layout::CrossAlign::center);
    auto header_row = layout::row({widgets::node(header), end_row});
    header_row->fill_main(true).main_align(layout::MainAlign::space_between).cross_align(layout::CrossAlign::center);
    header_row->self_align = layout::CrossAlign::stretch;

    std::vector<layout::NodePtr> left;
    std::vector<layout::NodePtr> right;
    build_left(left);
    build_right(right);
    build_dialog();
    auto left_column = layout::column(std::move(left));
    left_column->spacing(layout.row_gap);
    auto right_column = layout::column(std::move(right));
    right_column->spacing(layout.row_gap);
    auto columns = layout::row({left_column, right_column});
    columns->spacing(layout.column_space);

    const double symbol_pitch = layout.symbol_size + layout.symbol_gap;
    auto symbols = std::make_shared<layout::Leaf>(
        [this, symbol_pitch](const layout::Constraints& c) {
            symbols_per_line = static_cast<std::size_t>(std::max(1.0, std::floor(c.max_width / symbol_pitch)));
            const std::size_t lines = (res::all_icons.size() + symbols_per_line - 1) / symbols_per_line;
            return layout::Size{static_cast<double>(symbols_per_line) * symbol_pitch, static_cast<double>(lines) * symbol_pitch};
        },
        [this](const render::Rect& r) { symbols_at = {r.x, r.y}; });
    auto page = layout::column({header_row, columns, captioned("Symbols", symbols)});
    page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin)).fill_cross(true);

    root = page;
    if (host.client_title_bar()) {
        title_bar = std::make_unique<widgets::TitleBar>(widgets::TitleBarOptions{"lamina-ui components", icon_font}, fonts);
        page->self_align = layout::CrossAlign::stretch;
        auto framed = layout::column({widgets::node(*title_bar), layout::weighted(page, 1.0)});
        framed->fill_cross(true);
        root = framed;
    }
    overlay.add_base(controls, chrome);
    overlay.set_focus(focus);
}

template <typename W, typename... Args>
W& Gallery::Impl::control(std::vector<layout::NodePtr>& row, Args&&... args) {
    W& w = controls.emplace<W>(std::forward<Args>(args)...);
    focus.add(w);
    enablers[w.accessible().name] = [&w](bool e) { w.set_enabled(e); };
    row.push_back(widgets::node(w));
    return w;
}

template <typename W, typename OnResult, typename... Args>
W& Gallery::Impl::control_with(std::vector<layout::NodePtr>& row, OnResult on_result, Args&&... args) {
    W& w = controls.emplace_with<W>(std::move(on_result), std::forward<Args>(args)...);
    if constexpr (widgets::Focusable<W>) {
        focus.add(w);
    }
    if constexpr (widgets::Disableable<W>) {
        enablers[w.accessible().name] = [&w](bool e) { w.set_enabled(e); };
    }
    row.push_back(widgets::node(w));
    return w;
}

layout::NodePtr Gallery::Impl::items(std::vector<layout::NodePtr> nodes) {
    auto r = layout::row(std::move(nodes));
    r->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
    return r;
}

layout::NodePtr Gallery::Impl::captioned(const char* caption, layout::NodePtr content) {
    const auto style = tokens::text_style(tokens::type_style(layout.caption_type));
    Row row{std::make_unique<widgets::Label>(widgets::LabelOptions{caption, layout.caption_type, layout.caption, false}, fonts), content};
    auto r = layout::row({examples::caption_cell(*row.caption, layout.label_width, style.line_height, style.line_height), content});
    r->cross_align(layout::CrossAlign::center);
    rows.push_back(std::move(row));
    return r;
}

void Gallery::Impl::add_tooltip(widgets::Tooltip tooltip) {
    tooltips.push_back(std::make_unique<widgets::Tooltip>(std::move(tooltip)));
}

void Gallery::Impl::build_left(std::vector<layout::NodePtr>& column) {
    auto clicked = [this](std::string what) {
        return [this, what](bool c) {
            if (c) {
                report(what);
            }
        };
    };

    std::vector<layout::NodePtr> buttons;
    const std::pair<ButtonStyle, std::pair<Symbol, const char*>> button_list[] = {
        {ButtonStyle::filled, {res::icons::add, "追加"}},  {ButtonStyle::tonal, {res::icons::send, "Send"}},
        {ButtonStyle::outlined, {res::icons::edit, "編集"}}, {ButtonStyle::elevated, {res::icons::favorite, "Like"}},
        {ButtonStyle::text, {res::icons::share, "共有"}},
    };
    for (const auto& [style, content] : button_list) {
        control_with<widgets::Button>(buttons, clicked(std::string("Button: ") + content.second),
                                      widgets::ButtonOptions{content.second, style, ButtonSize::small, ButtonShape::round, icon(content.first)},
                                      fonts);
    }
    control_with<widgets::Button>(
        buttons, clicked("Button: medium"),
        widgets::ButtonOptions{"保存", ButtonStyle::filled, ButtonSize::medium, ButtonShape::square, icon(res::icons::download)}, fonts);
    column.push_back(captioned("Buttons", items(std::move(buttons))));

    std::vector<layout::NodePtr> icon_buttons;
    const std::pair<IconButtonStyle, const char*> icon_styles[] = {
        {IconButtonStyle::filled, "Filled"}, {IconButtonStyle::tonal, "Tonal"}, {IconButtonStyle::outlined, "Outlined"},
        {IconButtonStyle::standard, "Standard"}};
    for (const auto& [style, name] : icon_styles) {
        auto& b = control_with<widgets::IconButton>(icon_buttons, clicked(std::string("Icon button: ") + name),
                                                    widgets::IconButtonOptions{icon(res::icons::settings), std::string(name) + " settings", style});
        add_tooltip(widgets::name_tooltip(b, fonts));
    }
    const std::pair<Symbol, const char*> toggles[] = {
        {res::icons::favorite, "Favorite"}, {res::icons::star, "Star"}, {res::icons::bookmark, "Bookmark"}, {res::icons::notifications, "Notify"}};
    for (std::size_t i = 0; i < std::size(icon_styles); ++i) {
        auto& b = control_with<widgets::IconButton>(
            icon_buttons,
            [this, name = std::string(toggles[i].second)](widgets::IconButton& w, bool c) {
                if (c) {
                    report(name + (w.checked() ? ": on" : ": off"));
                }
            },
            widgets::IconButtonOptions{icon(toggles[i].first), toggles[i].second, icon_styles[i].first, IconButtonSize::small,
                                       ButtonShape::round, IconButtonWidth::uniform, true});
        add_tooltip(widgets::name_tooltip(b, fonts));
    }
    column.push_back(captioned("Icon buttons", items(std::move(icon_buttons))));

    std::vector<layout::NodePtr> sizes;
    const IconButtonSize size_list[] = {IconButtonSize::extra_small, IconButtonSize::small, IconButtonSize::medium, IconButtonSize::large,
                                        IconButtonSize::extra_large};
    const char* size_names[] = {"Extra small", "Small", "Medium", "Large", "Extra large"};
    for (std::size_t i = 0; i < std::size(size_list); ++i) {
        const auto shape = i % 2 == 0 ? ButtonShape::round : ButtonShape::square;
        const auto width = i == 1 ? IconButtonWidth::wide : IconButtonWidth::uniform;
        control_with<widgets::IconButton>(sizes, clicked(std::string("Size: ") + size_names[i]),
                                          widgets::IconButtonOptions{icon(res::icons::palette), std::string("Palette, ") + size_names[i],
                                                                     IconButtonStyle::tonal, size_list[i], shape, width});
    }
    column.push_back(captioned("Sizes", items(std::move(sizes))));

    std::vector<layout::NodePtr> fabs;
    const std::tuple<tokens::FabSize, Symbol, const char*> fab_list[] = {
        {tokens::FabSize::baseline, res::icons::edit, "Edit"}, {tokens::FabSize::medium, res::icons::add, "Add"},
        {tokens::FabSize::large, res::icons::mail, "Compose"}};
    for (const auto& [size, symbol, name] : fab_list) {
        control_with<widgets::Fab>(fabs, clicked(std::string("FAB: ") + name), widgets::FabOptions{icon(symbol), name, size});
    }
    control_with<widgets::Fab>(fabs, clicked("FAB: secondary"),
                               widgets::FabOptions{icon(res::icons::search), "Find", tokens::FabSize::baseline, tokens::FabStyle::secondary_container});
    column.push_back(captioned("FAB", items(std::move(fabs))));

    std::vector<layout::NodePtr> switches;
    auto switched = [this](widgets::Switch& w, bool c) {
        if (c) {
            report(w.checked() ? "Switch: on" : "Switch: off");
        }
    };
    control_with<widgets::Switch>(switches, switched, widgets::SwitchOptions{.accessible_name = "Wi-Fi"});
    control_with<widgets::Switch>(switches, switched,
                                  widgets::SwitchOptions{.checked = true, .checked_icon = icon(res::icons::check), .accessible_name = "Bluetooth"});
    control_with<widgets::Switch>(switches, switched,
                                  widgets::SwitchOptions{.checked_icon = icon(res::icons::check), .unchecked_icon = icon(res::icons::close),
                                                         .accessible_name = "Location"});
    column.push_back(captioned("Switch", items(std::move(switches))));

    std::vector<layout::NodePtr> checkboxes;
    const std::pair<tokens::CheckboxState, const char*> boxes[] = {
        {tokens::CheckboxState::off, "Photos"}, {tokens::CheckboxState::on, "Videos"}, {tokens::CheckboxState::indeterminate, "All media"}};
    for (const auto& [state, name] : boxes) {
        control_with<widgets::Checkbox>(
            checkboxes,
            [this](widgets::Checkbox& w, bool c) {
                if (c) {
                    report(std::string("Checkbox: ") + checkbox_name(w.state()));
                }
            },
            widgets::CheckboxOptions{state, true, name});
    }
    column.push_back(captioned("Checkbox", items(std::move(checkboxes))));

    std::vector<layout::NodePtr> radio_row;
    const char* radio_names[] = {"Small", "Medium", "Large"};
    for (int i = 0; i < 3; ++i) {
        auto& r = control_with<widgets::RadioButton>(
            radio_row,
            [this, i](widgets::RadioButton&, bool c) {
                if (!c) {
                    return;
                }
                for (int k = 0; k < static_cast<int>(radios.size()); ++k) {
                    radios[static_cast<std::size_t>(k)]->set_selected(k == i);
                }
                report("Radio: " + std::to_string(i + 1));
            },
            widgets::RadioButtonOptions{i == 0, true, radio_names[i]});
        radios.push_back(&r);
    }
    column.push_back(captioned("Radio button", items(std::move(radio_row))));

    std::vector<layout::NodePtr> sliders;
    auto moved = [this](widgets::Slider& s, bool changed) {
        if (changed) {
            report("Slider: " + std::to_string(static_cast<int>(std::lround(s.value() * 100.0))) + " %");
        }
    };
    auto& continuous = controls.emplace_with<widgets::Slider>(moved, widgets::SliderOptions{.value = 0.4, .accessible_name = "Volume"});
    continuous.set_width(layout.slider_width);
    focus.add(continuous);
    sliders.push_back(widgets::centered_node(continuous));
    auto& stepped = controls.emplace_with<widgets::Slider>(moved, widgets::SliderOptions{.value = 0.5, .steps = 3, .accessible_name = "Brightness"});
    stepped.set_width(layout.slider_width);
    focus.add(stepped);
    sliders.push_back(widgets::centered_node(stepped));
    column.push_back(captioned("Slider", items(std::move(sliders))));
}

void Gallery::Impl::build_right(std::vector<layout::NodePtr>& column) {
    std::vector<layout::NodePtr> fields;
    auto& name = controls.emplace<widgets::TextField>(
        widgets::TextFieldOptions{.label = "Name", .placeholder = "Jane Doe", .clipboard = host.clipboard()}, fonts);
    focus.add(name);
    fields.push_back(widgets::node(name));
    auto& search = controls.emplace<widgets::TextField>(widgets::TextFieldOptions{.placeholder = "Search", .clipboard = host.clipboard()}, fonts);
    focus.add(search);
    fields.push_back(widgets::node(search));
    auto field_column = layout::column(std::move(fields));
    field_column->spacing(layout.field_gap);
    column.push_back(captioned("Text field", field_column));

    auto& tabs = controls.emplace_with<widgets::TabRow>(
        [this](widgets::TabRow& w, bool c) {
            if (c) {
                report("Tab: " + std::to_string(w.selected() + 1));
            }
        },
        widgets::TabRowOptions{{{"Flights", icon(res::icons::send)}, {"Trips", icon(res::icons::bookmark), false}, {"Explore", icon(res::icons::search)}},
                               tokens::TabStyle::primary,
                               0,
                               widgets::TabIconPosition::leading,
                               "Travel"},
        fonts);
    tabs.set_width(layout.tabs_width);
    focus.add(tabs);
    column.push_back(captioned("Tabs", widgets::centered_node(tabs)));

    std::vector<layout::NodePtr> overlays;
    menu = std::make_unique<widgets::Menu>(
        widgets::MenuOptions{{{"Edit", icon(res::icons::edit)},
                              {"Share", icon(res::icons::share), std::nullopt, false},
                              {"Copy", icon(res::icons::content_copy)},
                              {"Settings", icon(res::icons::settings)}},
                             "Actions"},
        fonts);
    menu_button = &control_with<widgets::Button>(
        overlays,
        [this](bool c) {
            if (c) {
                open_menu();
            }
        },
        widgets::ButtonOptions{"Menu", ButtonStyle::outlined, ButtonSize::small, ButtonShape::round, icon(res::icons::more_vert)}, fonts);
    dialog_button = &control_with<widgets::Button>(
        overlays,
        [this](bool c) {
            if (c) {
                open_dialog();
            }
        },
        widgets::ButtonOptions{"Dialog", ButtonStyle::tonal, ButtonSize::small, ButtonShape::round, icon(res::icons::notifications)}, fonts);
    column.push_back(captioned("Menu, dialog", items(std::move(overlays))));

    std::vector<layout::NodePtr> tips;
    auto& plain = control<widgets::Button>(tips, widgets::ButtonOptions{"Hover me", ButtonStyle::text}, fonts);
    widgets::Tooltip plain_tip({.text = "Plain tooltip with a caret", .caret = true}, fonts);
    plain_tip.anchor_to(plain);
    add_tooltip(std::move(plain_tip));
    auto& info = control<widgets::IconButton>(tips, widgets::IconButtonOptions{icon(res::icons::info), "About photos", IconButtonStyle::standard});
    widgets::Tooltip rich_tip({.text = "Photos are grouped by the week they were taken. Pick one to see every photo from that week.",
                               .kind = widgets::TooltipKind::rich,
                               .title = "Grouped photos",
                               .action_label = "Learn more",
                               .persistent = true},
                              fonts);
    rich_tip.anchor_to(info);
    add_tooltip(std::move(rich_tip));
    column.push_back(captioned("Tooltip", items(std::move(tips))));

    std::vector<layout::NodePtr> samples;
    for (const TypeSample& sample : type_samples) {
        auto& plain_label = chrome.emplace<widgets::Label>(widgets::LabelOptions{sample.name, sample.role, layout.text, false}, fonts);
        auto& strong_label =
            chrome.emplace<widgets::Label>(widgets::LabelOptions{sample.name, tokens::emphasized(sample.role), layout.text, false}, fonts);
        auto pair = layout::row({widgets::node(plain_label), widgets::node(strong_label)});
        pair->spacing(layout.type_gap).cross_align(layout::CrossAlign::end);
        samples.push_back(pair);
    }
    auto type_column = layout::column(std::move(samples));
    type_column->spacing(layout.field_gap);
    column.push_back(captioned("Type", type_column));

    std::vector<layout::NodePtr> theme_controls = chips.nodes();
    dark_toggle = &chrome.emplace_with<widgets::IconButton>(
        [this](widgets::IconButton& w, bool c) {
            if (c) {
                options.dark = w.checked();
                retheme();
            }
        },
        widgets::IconButtonOptions{icon(res::icons::dark_mode), "Dark theme", IconButtonStyle::outlined, IconButtonSize::small, ButtonShape::round,
                                   IconButtonWidth::uniform, true});
    add_tooltip(widgets::name_tooltip(*dark_toggle, fonts));
    focus.add(chips);
    focus.add(*dark_toggle);
    theme_controls.push_back(widgets::node(*dark_toggle));
    auto chips_row = layout::row(std::move(theme_controls));
    chips_row->spacing(chips.style().gap).cross_align(layout::CrossAlign::center);
    column.push_back(captioned("Theme", chips_row));
}

void Gallery::Impl::build_dialog() {
    std::vector<layout::NodePtr> nodes;
    const char* topics[] = {"Messages", "Mentions", "Weekly summary"};
    for (const char* topic : topics) {
        auto& box = dialog_widgets.emplace<widgets::Checkbox>(widgets::CheckboxOptions{tokens::CheckboxState::on, true, topic});
        dialog_focus.add(box);
        auto& label = dialog_widgets.emplace<widgets::Label>(widgets::LabelOptions{topic, TypeRole::body_large, color::Role::on_surface, false}, fonts);
        auto r = layout::row({widgets::node(box), widgets::node(label)});
        r->cross_align(layout::CrossAlign::center);
        nodes.push_back(r);
    }
    auto& field = dialog_widgets.emplace<widgets::TextField>(
        widgets::TextFieldOptions{.label = "Email", .placeholder = "name@example.com", .clipboard = host.clipboard()}, fonts);
    dialog_focus.add(field);
    nodes.push_back(widgets::node(field));
    for (int i = 0; i < layout.dialog_paragraphs; ++i) {
        auto& p = dialog_widgets.emplace<widgets::Label>(
            widgets::LabelOptions{"Paragraph " + std::to_string(i + 1) +
                                      ": notifications arrive on every device you are signed in to, and you can change this later.",
                                  TypeRole::body_medium, color::Role::on_surface_variant, true},
            fonts);
        nodes.push_back(widgets::node(p));
    }
    auto content = layout::column(std::move(nodes));
    content->spacing(layout.field_gap).fill_cross(true);
    dialog = std::make_unique<widgets::Dialog>(
        widgets::DialogOptions{"Notification settings", "Choose what you hear about.", "Save", "Cancel", icon(res::icons::notifications),
                               std::nullopt, widgets::DialogContent{content, &dialog_widgets, &dialog_focus}},
        fonts);
}

void Gallery::Impl::retheme() {
    theme.fade_to(color::spec_2021().build(options));
    dark_toggle->set_icon(icon(options.dark ? res::icons::light_mode : res::icons::dark_mode));
}

void Gallery::Impl::open_menu() {
    menu->open(menu_button->bounds(), window, source);
    overlay.push(*menu, [this](const widgets::MenuResponse& r) {
        if (r.chosen) {
            report("Menu: item " + std::to_string(*r.chosen + 1));
        }
    });
}

void Gallery::Impl::open_dialog() {
    dialog->open(window, source);
    overlay.push(*dialog, [this](widgets::DialogResult r) {
        if (r == widgets::DialogResult::confirmed) {
            report("Dialog: saved");
        } else if (r == widgets::DialogResult::dismissed) {
            report("Dialog: cancelled");
        }
    });
}

Gallery::Gallery(const text::FontStack& fonts, const icons::IconFont& icon_font, GalleryHost& host)
    : impl_(std::make_unique<Impl>(fonts, icon_font, host)) {}

Gallery::~Gallery() = default;

color::Argb Gallery::update(const sdl3::Frame& frame) {
    Impl& g = *impl_;
    g.window = {frame.width_dp, frame.height_dp};
    if (g.title_bar) {
        g.title_bar->set_maximized(frame.maximized);
        g.title_bar->set_active(frame.active);
    }
    layout::run(*g.root, {0.0, 0.0, frame.width_dp, frame.height_dp});
    if (g.title_bar) {
        g.host.set_caption_regions(g.title_bar->regions());
    }
    if (g.dialog->is_open()) {
        g.dialog->set_window(g.window);
    }
    for (auto& t : g.tooltips) {
        t->set_window(g.window);
    }
    g.scheme = g.theme.current();

    for (const auto& e : frame.pointer) {
        g.source = input::InputSource::pointer;
        if (g.overlay.handle(e)) {
            continue;
        }
        bool consumed = false;
        for (auto& t : g.tooltips) {
            const widgets::TooltipResponse r = t->handle(e);
            if (r.action) {
                g.report("Tooltip: learn more");
            }
            consumed = consumed || r.consumed;
        }
        if (consumed) {
            continue;
        }
        if (g.title_bar) {
            switch (g.title_bar->handle(e)) {
            case widgets::TitleBarAction::minimize:
                g.host.minimize();
                break;
            case widgets::TitleBarAction::toggle_maximize:
                g.host.toggle_maximize();
                break;
            case widgets::TitleBarAction::close:
                g.host.close();
                break;
            case widgets::TitleBarAction::none:
                break;
            }
        }
        if (const auto seed = g.chips.handle(e)) {
            g.options.seed = *seed;
            g.retheme();
        }
        g.controls.handle(e);
        g.chrome.handle(e);
    }
    for (const auto& w : frame.wheel) {
        g.overlay.handle(w);
    }
    for (const auto& k : frame.keys) {
        g.source = input::InputSource::keyboard;
        if (g.overlay.handle(k)) {
            continue;
        }
        bool consumed = false;
        for (auto& t : g.tooltips) {
            const widgets::TooltipResponse r = t->handle(k);
            consumed = consumed || r.consumed;
        }
        if (consumed || g.focus.handle(k)) {
            continue;
        }
        if (const auto seed = g.chips.handle(k)) {
            g.options.seed = *seed;
            g.retheme();
        }
        g.controls.handle(k);
        g.chrome.handle(k);
    }
    for (const auto& t : frame.text) {
        if (!g.overlay.handle(t)) {
            g.controls.handle(t);
        }
    }
    return g.scheme[color::Role::surface];
}

bool Gallery::paint(render::Canvas& canvas, const sdl3::Frame& frame) {
    Impl& g = *impl_;
    const double dt = frame.dt_seconds;
    g.theme.step(dt);
    g.chips.step(dt);
    g.controls.step(dt);
    g.chrome.step(dt);
    g.menu->step(dt);
    g.dialog->step(dt);
    for (auto& t : g.tooltips) {
        t->step(dt);
    }
    if (g.title_bar) {
        g.title_bar->step(dt);
        g.title_bar->paint(canvas, g.scheme);
    }
    const color::ColorScheme& s = g.scheme;
    g.header.paint(canvas, s);
    g.status.paint(canvas, s);
    for (const Row& row : g.rows) {
        row.caption->paint(canvas, s);
    }
    const double pitch = g.layout.symbol_size + g.layout.symbol_gap;
    for (std::size_t i = 0; i < res::all_icons.size(); ++i) {
        const auto col = static_cast<double>(i % g.symbols_per_line);
        const auto line = static_cast<double>(i / g.symbols_per_line);
        g.icon_font.draw(canvas, res::all_icons[i], {g.symbols_at.x + col * pitch, g.symbols_at.y + line * pitch}, g.layout.symbol_size,
                         s[g.layout.caption]);
    }
    g.chips.paint(canvas, s);
    g.chrome.paint(canvas, s);
    g.controls.paint(canvas, s);
    g.menu->paint(canvas, s);
    g.dialog->paint(canvas, s);
    for (const auto& t : g.tooltips) {
        t->paint(canvas, s);
    }
    bool tooltips_moving = false;
    for (const auto& t : g.tooltips) {
        tooltips_moving = tooltips_moving || t->animating();
    }
    return !g.theme.settled() || g.chips.animating() || g.controls.animating() || g.chrome.animating() || g.menu->animating() ||
           g.dialog->animating() || tooltips_moving || (g.title_bar && g.title_bar->animating());
}

std::optional<render::Rect> Gallery::caret_area() const {
    if (const auto caret = impl_->dialog->caret_area()) {
        return caret;
    }
    return impl_->focus.caret_area();
}

std::optional<double> Gallery::next_wake_seconds() const {
    std::optional<double> wake = impl_->focus.next_wake_seconds();
    auto merge = [&wake](std::optional<double> w) {
        if (w) {
            wake = wake ? std::min(*wake, *w) : *w;
        }
    };
    merge(impl_->dialog->next_wake_seconds());
    for (const auto& t : impl_->tooltips) {
        merge(t->next_wake_seconds());
    }
    return wake;
}

std::vector<widgets::AccessibleNode> Gallery::accessible_nodes() const {
    std::vector<widgets::AccessibleNode> nodes = impl_->chrome.accessible_nodes();
    for (widgets::AccessibleNode& n : impl_->controls.accessible_nodes()) {
        nodes.push_back(std::move(n));
    }
    if (auto m = impl_->menu->accessible()) {
        nodes.push_back(std::move(*m));
    }
    if (auto d = impl_->dialog->accessible()) {
        nodes.push_back(std::move(*d));
    }
    for (const auto& t : impl_->tooltips) {
        if (auto n = t->accessible()) {
            nodes.push_back(std::move(*n));
        }
    }
    return nodes;
}

std::optional<render::Rect> Gallery::find(std::string_view name) const {
    const std::vector<widgets::AccessibleNode> nodes = accessible_nodes();
    auto node = widgets::find_accessible(
        nodes, [name](const widgets::AccessibleNode& n) { return n.name == name && n.role != widgets::AccessibleRole::label; });
    if (!node) {
        node = widgets::find_accessible(nodes, name);
    }
    return node ? std::optional<render::Rect>(node->bounds) : std::nullopt;
}

bool Gallery::set_enabled(std::string_view name, bool enabled) {
    const auto it = impl_->enablers.find(name);
    if (it == impl_->enablers.end()) {
        return false;
    }
    it->second(enabled);
    return true;
}

void Gallery::set_dark(bool dark) {
    Impl& g = *impl_;
    if (g.options.dark == dark) {
        return;
    }
    g.options.dark = dark;
    g.dark_toggle->set_checked(dark);
    g.retheme();
}

}
