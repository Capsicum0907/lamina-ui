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
#include "components/pages.hpp"
#include "components/reflow.hpp"
#include "common/theme.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/layout/safe_area.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/motion.hpp"
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
#include "lamina/widgets/scroll_area.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/tokens/tabs.hpp"
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
    double line_gap = 8.0;
    double caption_gap = 8.0;
    double type_gap = 24.0;
    color::Role text = color::Role::on_surface;
    color::Role caption = color::Role::on_surface_variant;
    TypeRole header_type = TypeRole::headline_small_emphasized;
    TypeRole caption_type = TypeRole::label_large;
    double symbol_size = 24.0;
    double symbol_gap = 12.0;
    int dialog_paragraphs = 10;
    double title_top = 16.0;
    color::Role veil = color::Role::surface;
    motion::SpringSpec veil_spring = tokens::motion_scheme.fast_effects();
    double veil_swap_at = 0.99;
    double status_width = 150.0;
    double status_gap = 24.0;
    std::string font_name_prefix = "Font, ";
    std::string font_report_prefix = "Font: ";
    std::string width_report_prefix = "Width: ";
    std::array<const char*, 16> page_names{"Basics", "Chips", "Lists", "App bars", "Inputs", "Dates", "Times", "Sheets", "Drawers", "Search",
                                           "Groups", "Progress", "Toolbars", "Adaptive", "Desktop", "Viewer"};
};

class ScrolledBody final : public layout::Node {
public:
    ScrolledBody(layout::NodePtr content, std::vector<widgets::ScrollArea>& scrolls, const std::size_t& current)
        : content_(std::move(content)), scrolls_(scrolls), current_(current) {}

    widgets::ScrollArea& scroll() const { return scrolls_[current_]; }

protected:
    layout::Size on_measure(const layout::Constraints& c) override {
        layout::Size s = content_->measure({0.0, c.max_width, 0.0, layout::unbounded});
        if (!std::isfinite(s.height)) {
            s = content_->measure({0.0, c.max_width, 0.0, c.max_height});
        }
        content_height_ = s.height;
        return c.constrain({std::isfinite(c.max_width) ? c.max_width : s.width, std::isfinite(c.max_height) ? c.max_height : s.height});
    }
    void on_arrange(const render::Rect& r) override {
        widgets::ScrollArea& a = scroll();
        a.set_viewport(r);
        a.set_content_height(content_height_);
        content_->arrange({r.x, r.y - a.offset(), r.width, content_height_});
    }

private:
    layout::NodePtr content_;
    std::vector<widgets::ScrollArea>& scrolls_;
    const std::size_t& current_;
    double content_height_ = 0.0;
};

class PageSwitch final : public layout::Node {
public:
    PageSwitch(std::vector<layout::NodePtr> pages, const std::size_t& current) : pages_(std::move(pages)), current_(current) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return pages_[current_]->measure(c); }
    void on_arrange(const render::Rect& r) override { pages_[current_]->arrange(r); }

private:
    std::vector<layout::NodePtr> pages_;
    const std::size_t& current_;
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

struct GalleryState {
    std::size_t page = 0;
    color::SchemeOptions options = color::default_scheme_options;
    bool enabled = true;
    bool font_focused = false;
    std::optional<layout::WindowWidthClass> reported_width;
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
    Impl(const text::FontStack& f, const GalleryFonts* typefaces, const icons::IconFont& i, GalleryHost& h, const GalleryState& state);

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
    void open_font_menu();
    void open_dialog();
    GalleryState state() const;
    void switch_page(std::size_t next);
    void show_pending_page();
    void paint_veil(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void handle_title_bar(const input::PointerEvent& e);
    void set_size_class(const layout::WindowSizeClass& c);
    void place(const sdl3::Frame& frame);
    input::PointerEvent page_event(const input::PointerEvent& e);
    widgets::ScrollArea& scroll() { return page_box->scroll(); }
    void reveal_focus();
    bool route(const input::KeyEvent& k);
    widgets::FocusManager& current_focus() { return page == 0 ? focus : pages[page - 1]->focus(); }
    Page* current_page() const { return page == 0 ? nullptr : pages[page - 1].get(); }

    const Layout layout;
    text::FontStack fonts;
    icons::IconFont icon_font;
    GalleryHost& host;
    std::optional<GalleryFonts> typefaces;
    std::optional<text::Typeface> pending_typeface;
    color::SchemeOptions options;
    ThemeFade theme;
    color::ColorScheme scheme;
    SeedChips chips;
    widgets::Label header;
    widgets::Label status;
    render::Rect status_box{0.0, 0.0, 0.0, 0.0};
    widgets::WidgetSet controls;
    widgets::WidgetSet chrome;
    widgets::WidgetSet header_widgets;
    widgets::FocusManager focus;
    widgets::OverlayStack overlay;
    std::vector<Row> rows;
    std::vector<std::unique_ptr<widgets::Tooltip>> tooltips;
    widgets::Switch* enabled_switch = nullptr;
    widgets::TabRow* page_tabs = nullptr;
    std::vector<std::unique_ptr<Page>> pages;
    std::size_t page = 0;
    std::optional<std::size_t> pending_page;
    motion::Animated veil{0.0, layout.veil_spring};
    std::shared_ptr<ScrolledBody> page_box;
    widgets::IconButton* dark_toggle = nullptr;
    widgets::Button* menu_button = nullptr;
    widgets::Button* dialog_button = nullptr;
    std::unique_ptr<widgets::Menu> menu;
    widgets::Button* font_button = nullptr;
    std::unique_ptr<widgets::Menu> font_menu;
    widgets::WidgetSet dialog_widgets;
    widgets::FocusManager dialog_focus;
    std::unique_ptr<widgets::Dialog> dialog;
    std::unique_ptr<widgets::TitleBar> title_bar;
    std::vector<widgets::RadioButton*> radios;
    std::map<std::string, std::function<void(bool)>, std::less<>> enablers;
    std::vector<render::Point> symbol_spots;
    layout::NodePtr root;
    input::InputSource source = input::InputSource::pointer;
    layout::Size window{static_cast<double>(window_width_dp), static_cast<double>(window_height_dp)};
    layout::WindowSizeClass size_class = layout::compute_window_size_class(window.width, window.height);
    Reflow reflow{size_class};
    std::vector<widgets::ScrollArea> scrolls;
    std::optional<layout::WindowWidthClass> reported_width;
    bool pointer_held = false;
    std::string revealed_focus;
};

Gallery::Impl::Impl(const text::FontStack& f, const GalleryFonts* typefaces_, const icons::IconFont& i, GalleryHost& h,
                    const GalleryState& state)
    : fonts(f),
      icon_font(i),
      host(h),
      typefaces(typefaces_ ? std::optional<GalleryFonts>(*typefaces_) : std::nullopt),
      options(state.options),
      theme(color::spec_2021().build(options)),
      scheme(theme.current()),
      header({"部品", layout.header_type, layout.text, false}, f),
      status({"—", layout.caption_type, layout.caption, false}, f),
      reported_width(state.reported_width) {
    auto& switch_label = header_widgets.emplace<widgets::Label>(widgets::LabelOptions{"Enabled", layout.caption_type, layout.caption, false}, fonts);
    enabled_switch = &header_widgets.emplace_with<widgets::Switch>(
        [this](widgets::Switch& s, bool clicked) {
            if (clicked) {
                controls.set_enabled(s.checked());
                for (auto& p : pages) {
                    p->set_enabled(s.checked());
                }
                report(s.checked() ? "Enabled: on" : "Enabled: off");
            }
        },
        widgets::SwitchOptions{.checked = state.enabled, .checked_icon = icon(res::icons::check), .accessible_name = "Enabled"});
    focus.add(*enabled_switch);
    std::vector<widgets::TabItem> page_items;
    for (const char* name : layout.page_names) {
        page_items.push_back({name});
    }
    page_tabs = &header_widgets.emplace_with<widgets::TabRow>(
        [this](widgets::TabRow& w, bool changed) {
            if (changed) {
                switch_page(w.selected());
            }
        },
        widgets::TabRowOptions{.tabs = page_items,
                               .style = tokens::TabStyle::secondary,
                               .selected = state.page,
                               .accessible_name = "Pages",
                               .scrollable = true,
                               .edge_padding = layout.margin - tokens::tab_tokens.horizontal_text_padding},
        f);
    page = state.page;
    focus.add(*page_tabs);
    PageContext context{f, i, [this](std::string s) { report(std::move(s)); },
                        [this](widgets::FocusManager& m) {
                            m.add(*enabled_switch);
                            m.add(*page_tabs);
                        },
                        &size_class};
    pages.push_back(make_chips_page(context));
    pages.push_back(make_lists_page(context));
    pages.push_back(make_bars_page(context));
    pages.push_back(make_inputs_page(context));
    pages.push_back(make_dates_page(context));
    pages.push_back(make_times_page(context));
    pages.push_back(make_sheets_page(context));
    pages.push_back(make_drawers_page(context));
    pages.push_back(make_search_page(context));
    pages.push_back(make_groups_page(context));
    pages.push_back(make_progress_page(context));
    pages.push_back(make_toolbars_page(context));
    pages.push_back(make_adaptive_page(context));
    pages.push_back(make_desktop_page(context));
    pages.push_back(make_viewer_page(context));

    auto toggle_row = layout::row({widgets::node(switch_label), widgets::node(*enabled_switch)});
    toggle_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);
    auto status_cell = std::make_shared<layout::Leaf>(
        [this, toggle_row](const layout::Constraints& c) {
            const double toggle = toggle_row->measure({}).width;
            const double room = std::max(0.0, c.max_width - toggle - layout.status_gap);
            return layout::Size{std::min(layout.status_width, room), status.measure(c).height};
        },
        [this](const render::Rect& r) {
            status_box = r;
            status.arrange(r);
        });
    auto end_row = layout::row({status_cell, toggle_row});
    end_row->spacing(layout.status_gap).cross_align(layout::CrossAlign::center);
    auto header_row = layout::row({widgets::node(header), end_row});
    header_row->fill_main(true).main_align(layout::MainAlign::space_between).cross_align(layout::CrossAlign::center);
    header_row->padding({layout.margin, 0.0, layout.margin, 0.0});
    header_row->self_align = layout::CrossAlign::stretch;

    std::vector<layout::NodePtr> left;
    std::vector<layout::NodePtr> right;
    build_left(left);
    build_right(right);
    build_dialog();
    auto columns = std::make_shared<Columns>(reflow, std::vector<std::vector<layout::NodePtr>>{left, right}, layout.column_space, layout.row_gap);

    symbol_spots.resize(res::all_icons.size());
    const double symbol_pitch = layout.symbol_size + layout.symbol_gap;
    std::vector<layout::NodePtr> symbol_nodes;
    for (std::size_t k = 0; k < symbol_spots.size(); ++k) {
        symbol_nodes.push_back(layout::fixed({symbol_pitch, symbol_pitch}, [this, k](const render::Rect& r) { symbol_spots[k] = {r.x, r.y}; }));
    }
    auto symbols = reflow.flow(std::move(symbol_nodes));
    auto basics = layout::column({columns, captioned("Symbols", symbols)});
    basics->spacing(layout.row_gap);
    std::vector<layout::NodePtr> bodies{reflow.root(basics)};
    for (const auto& p : pages) {
        bodies.push_back(p->body());
    }
    scrolls.resize(bodies.size());
    auto page_content = layout::column({std::make_shared<PageSwitch>(std::move(bodies), page)});
    page_content->padding({layout.margin, layout.row_gap, layout.margin, layout.margin}).fill_cross(true);
    page_box = std::make_shared<ScrolledBody>(page_content, scrolls, page);
    page_box->self_align = layout::CrossAlign::stretch;
    auto content = layout::column({header_row, widgets::full_width_node(*page_tabs), page_box});
    content->padding({0.0, layout.title_top, 0.0, 0.0}).fill_cross(true);

    root = content;
    if (host.client_title_bar()) {
        title_bar = std::make_unique<widgets::TitleBar>(widgets::TitleBarOptions{"lamina-ui components", icon_font}, fonts);
        content->self_align = layout::CrossAlign::stretch;
        auto framed = layout::column({widgets::node(*title_bar), layout::weighted(content, 1.0)});
        framed->fill_cross(true);
        root = framed;
    }
    overlay.add_base(controls, chrome, header_widgets);
    overlay.set_focus(focus);
    chips.select(options.seed);
    if (options.dark) {
        dark_toggle->set_checked(true);
        dark_toggle->set_icon(icon(res::icons::light_mode));
    }
    host.set_dark_theme(options.dark);
    if (!state.enabled) {
        controls.set_enabled(false);
        for (auto& p : pages) {
            p->set_enabled(false);
        }
    }
    if (state.font_focused && font_button) {
        font_button->set_focused(true);
    }
}

GalleryState Gallery::Impl::state() const {
    return {page, options, enabled_switch->checked(), font_button && font_button->focused(), reported_width};
}

void Gallery::Impl::set_size_class(const layout::WindowSizeClass& c) {
    size_class = c;
    const layout::WindowWidthClass w = c.width_class();
    if (reported_width != w) {
        reported_width = w;
        report(layout.width_report_prefix + std::string(layout::name(w)));
    }
}

input::PointerEvent Gallery::Impl::page_event(const input::PointerEvent& e) {
    const bool inside = e.action != input::PointerAction::leave && (scroll().offset() <= 0.0 || widgets::contains(page_box->rect(), e.x_dp, e.y_dp));
    const bool held = pointer_held;
    if (e.action == input::PointerAction::down) {
        pointer_held = inside;
    } else if (e.action == input::PointerAction::up) {
        pointer_held = false;
    }
    if (inside || held) {
        return e;
    }
    return {input::PointerAction::leave, 0.0, 0.0};
}

void Gallery::Impl::place(const sdl3::Frame& frame) {
    layout::run(*root, layout::inset_rect({0.0, 0.0, frame.width_dp, frame.height_dp}, frame.safe_area));
    if (title_bar) {
        host.set_caption_regions(title_bar->regions());
    }
}

bool Gallery::Impl::route(const input::KeyEvent& k) {
    if (Page* p = current_page()) {
        return p->handle(k) || header_widgets.handle(k);
    }
    if (overlay.handle(k)) {
        return true;
    }
    bool consumed = false;
    for (auto& t : tooltips) {
        consumed = t->handle(k).consumed || consumed;
    }
    if (consumed || focus.handle(k)) {
        return true;
    }
    bool used = false;
    if (const auto seed = chips.handle(k)) {
        options.seed = *seed;
        retheme();
        used = true;
    }
    used = controls.handle(k) || used;
    used = chrome.handle(k) || used;
    return header_widgets.handle(k) || used;
}

void Gallery::Impl::reveal_focus() {
    std::vector<widgets::AccessibleNode> nodes;
    if (Page* p = current_page()) {
        nodes = p->accessible_nodes();
    } else {
        nodes = chrome.accessible_nodes();
        for (widgets::AccessibleNode& n : controls.accessible_nodes()) {
            nodes.push_back(std::move(n));
        }
    }
    const auto f = widgets::focused_accessible(nodes);
    const std::string name = f ? f->name : std::string();
    if (name == revealed_focus) {
        return;
    }
    revealed_focus = name;
    if (!f || f->bounds.height <= 0.0) {
        return;
    }
    widgets::ScrollArea& a = scroll();
    const double top = f->bounds.y - a.viewport().y + a.offset();
    a.ensure_visible(top, top + f->bounds.height);
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
    auto f = reflow.flow(std::move(nodes));
    f->spacing(layout.column_gap).line_spacing(layout.line_gap).cross_align(layout::CrossAlign::center);
    return f;
}

layout::NodePtr Gallery::Impl::captioned(const char* caption, layout::NodePtr content) {
    const auto style = tokens::text_style(tokens::type_style(layout.caption_type));
    Row row{std::make_unique<widgets::Label>(widgets::LabelOptions{caption, layout.caption_type, layout.caption, false}, fonts), content};
    auto r = std::make_shared<Captioned>(reflow, examples::caption_cell(*row.caption, layout.label_width, style.line_height, style.line_height), content,
                                         layout.caption_gap);
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
    sliders.push_back(at_most(widgets::full_width_node(continuous), layout.slider_width));
    auto& stepped = controls.emplace_with<widgets::Slider>(moved, widgets::SliderOptions{.value = 0.5, .steps = 3, .accessible_name = "Brightness"});
    stepped.set_width(layout.slider_width);
    focus.add(stepped);
    sliders.push_back(at_most(widgets::full_width_node(stepped), layout.slider_width));
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
    focus.add(tabs);
    column.push_back(captioned("Tabs", at_most(widgets::full_width_node(tabs), layout.tabs_width)));

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
        auto pair = reflow.flow({widgets::node(plain_label), widgets::node(strong_label)});
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
    if (typefaces) {
        const text::TypefaceSpec& current = text::typeface_spec(typefaces->typeface);
        std::vector<widgets::MenuItem> items;
        for (const text::TypefaceSpec& t : text::typefaces) {
            items.push_back({std::string(t.name), icon(t.typeface == current.typeface ? res::icons::check : res::icons::text_fields)});
        }
        font_menu = std::make_unique<widgets::Menu>(widgets::MenuOptions{std::move(items), "Fonts"}, fonts);
        font_button = &chrome.emplace_with<widgets::Button>(
            [this](bool c) {
                if (c) {
                    open_font_menu();
                }
            },
            widgets::ButtonOptions{.label = std::string(current.name),
                                   .style = ButtonStyle::outlined,
                                   .leading_icon = icon(res::icons::text_fields),
                                   .accessible_name = layout.font_name_prefix + std::string(current.name)},
            fonts);
        focus.add(*font_button);
        theme_controls.push_back(widgets::node(*font_button));
    }
    auto chips_row = reflow.flow(std::move(theme_controls));
    chips_row->spacing(chips.style().gap).line_spacing(chips.style().gap).cross_align(layout::CrossAlign::center);
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
    host.set_dark_theme(options.dark);
}

void Gallery::Impl::handle_title_bar(const input::PointerEvent& e) {
    if (title_bar) {
        switch (title_bar->handle(e)) {
        case widgets::TitleBarAction::minimize:
            host.minimize();
            break;
        case widgets::TitleBarAction::toggle_maximize:
            host.toggle_maximize();
            break;
        case widgets::TitleBarAction::close:
            host.close();
            break;
        case widgets::TitleBarAction::none:
            break;
        }
    }
}

void Gallery::Impl::switch_page(std::size_t next) {
    if (next > pages.size() || next == pending_page.value_or(page)) {
        return;
    }
    pending_page = next;
    veil.animate_to(1.0);
}

void Gallery::Impl::paint_veil(render::Canvas& canvas, const color::ColorScheme& s) const {
    if (veil.value() > 0.0) {
        canvas.fill_rect(page_box->rect(), color::scale_alpha(s[layout.veil], std::min(1.0, veil.value())));
    }
}

void Gallery::Impl::show_pending_page() {
    if (!pending_page || veil.value() < layout.veil_swap_at) {
        return;
    }
    const std::size_t next = *pending_page;
    pending_page.reset();
    veil.animate_to(0.0);
    if (next == page) {
        return;
    }
    const bool tabs_focused = page_tabs->focused();
    current_focus().clear_focus();
    scroll().cancel_interaction();
    if (page == 0) {
        controls.cancel_interaction();
        for (auto& t : tooltips) {
            t->dismiss();
        }
    } else {
        pages[page - 1]->cancel_interaction();
    }
    page = next;
    if (Page* p = current_page()) {
        p->snap_layout();
    } else {
        reflow.snap();
    }
    if (tabs_focused) {
        page_tabs->set_focused(true);
    }
    report(std::string("Page: ") + layout.page_names[page]);
}

void Gallery::Impl::open_menu() {
    menu->open(menu_button->bounds(), window, source);
    overlay.push(*menu, [this](const widgets::MenuResponse& r) {
        if (r.chosen) {
            report("Menu: item " + std::to_string(*r.chosen + 1));
        }
    });
}

void Gallery::Impl::open_font_menu() {
    font_menu->open(font_button->bounds(), window, source);
    overlay.push(*font_menu, [this](const widgets::MenuResponse& r) {
        if (r.chosen && typefaces && text::typefaces[*r.chosen].typeface != typefaces->typeface) {
            pending_typeface = text::typefaces[*r.chosen].typeface;
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

namespace {

GalleryState started(GalleryStart start) {
    GalleryState state;
    state.options.dark = start.dark;
    return state;
}

}

Gallery::Gallery(const text::FontStack& fonts, const icons::IconFont& icon_font, GalleryHost& host, GalleryStart start)
    : impl_(std::make_unique<Impl>(fonts, nullptr, icon_font, host, started(start))) {}

Gallery::Gallery(GalleryFonts fonts, const icons::IconFont& icon_font, GalleryHost& host, GalleryStart start)
    : impl_(std::make_unique<Impl>(fonts.load(fonts.typeface), &fonts, icon_font, host, started(start))) {}

Gallery::~Gallery() = default;

color::Argb Gallery::update(const sdl3::Frame& frame) {
    if (impl_->pending_typeface && impl_->typefaces) {
        GalleryFonts next = *impl_->typefaces;
        next.typeface = *impl_->pending_typeface;
        const GalleryState state = impl_->state();
        const icons::IconFont icon_font = impl_->icon_font;
        GalleryHost& host = impl_->host;
        impl_ = std::make_unique<Impl>(next.load(next.typeface), &next, icon_font, host, state);
        impl_->report(impl_->layout.font_report_prefix + std::string(text::typeface_spec(next.typeface).name));
    }
    Impl& g = *impl_;
    g.show_pending_page();
    g.window = {frame.width_dp, frame.height_dp};
    g.set_size_class(frame.size_class);
    if (g.title_bar) {
        g.title_bar->set_maximized(frame.maximized);
        g.title_bar->set_active(frame.active);
    }
    g.place(frame);
    if (g.dialog->is_open()) {
        g.dialog->set_window(g.window);
    }
    for (auto& t : g.tooltips) {
        t->set_window(g.window);
    }
    for (auto& p : g.pages) {
        p->set_window(g.window);
    }
    g.scheme = g.theme.current();
    Page* current = g.current_page();

    if (current) {
        for (const auto& f : frame.fingers) {
            current->handle(f);
        }
    }
    for (const auto& e : frame.pointer) {
        g.source = input::InputSource::pointer;
        if (current) {
            g.handle_title_bar(e);
            g.scroll().route(e, [&](const input::PointerEvent& routed) {
                current->handle(g.page_event(routed));
                return current->holds_pointer();
            });
            g.header_widgets.handle(e);
            continue;
        }
        if (g.overlay.handle(e)) {
            continue;
        }
        bool tip_consumed = false;
        const bool scrolled = g.scroll().route(e, [&](const input::PointerEvent& routed) {
            const input::PointerEvent pe = g.page_event(routed);
            bool consumed = false;
            for (auto& t : g.tooltips) {
                const widgets::TooltipResponse r = t->handle(pe);
                if (r.action) {
                    g.report("Tooltip: learn more");
                }
                consumed = consumed || r.consumed;
            }
            if (consumed) {
                tip_consumed = true;
                return false;
            }
            if (const auto seed = g.chips.handle(pe)) {
                g.options.seed = *seed;
                g.retheme();
            }
            g.controls.handle(pe);
            g.chrome.handle(pe);
            return g.controls.holds_pointer() || g.chrome.holds_pointer();
        });
        if (tip_consumed && !scrolled) {
            continue;
        }
        if (!scrolled) {
            g.handle_title_bar(e);
        }
        g.header_widgets.handle(e);
    }
    for (const auto& c : frame.context) {
        g.source = input::InputSource::pointer;
        if (Page* p = g.current_page()) {
            p->handle(c);
        }
    }
    for (const auto& w : frame.wheel) {
        if (g.page_tabs->handle(w)) {
            continue;
        }
        if (g.current_page() ? g.current_page()->handle(w) : g.overlay.handle(w)) {
            continue;
        }
        g.scroll().handle(w);
    }
    for (const auto& k : frame.keys) {
        g.source = input::InputSource::keyboard;
        const bool typing = caret_area().has_value();
        const bool consumed = g.route(k) || typing;
        if (k.back && !k.repeat && !consumed) {
            g.host.system_back();
        }
    }
    if (!frame.keys.empty()) {
        g.reveal_focus();
    }
    for (const auto& t : frame.text) {
        if (Page* p = g.current_page()) {
            p->handle(t);
            continue;
        }
        if (!g.overlay.handle(t)) {
            g.controls.handle(t);
        }
    }
    if (!frame.pointer.empty() || !frame.context.empty() || !frame.wheel.empty() || !frame.keys.empty() || !frame.text.empty() || !frame.fingers.empty()) {
        g.place(frame);
    }
    return g.scheme[color::Role::surface];
}

bool Gallery::paint(render::Canvas& canvas, const sdl3::Frame& frame) {
    Impl& g = *impl_;
    const double dt = frame.dt_seconds;
    g.theme.step(dt);
    g.veil.step(dt);
    g.scroll().step(dt);
    Page* current = g.current_page();
    if (current) {
        current->step(dt);
    }
    g.chips.step(dt);
    g.controls.step(dt);
    g.chrome.step(dt);
    g.header_widgets.step(dt);
    g.menu->step(dt);
    if (g.font_menu) {
        g.font_menu->step(dt);
    }
    g.dialog->step(dt);
    for (auto& t : g.tooltips) {
        t->step(dt);
    }
    if (!current) {
        g.reflow.step(dt);
    }
    g.place(frame);
    if (g.title_bar) {
        g.title_bar->step(dt);
        g.title_bar->paint(canvas, g.scheme);
    }
    const color::ColorScheme& s = g.scheme;
    g.header.paint(canvas, s);
    canvas.save();
    canvas.push_clip(g.status_box);
    g.status.paint(canvas, s);
    canvas.pop_clip();
    canvas.restore();
    const render::Rect viewport = g.page_box->rect();
    if (current) {
        g.header_widgets.paint(canvas, s);
        canvas.save();
        canvas.push_clip(viewport);
        current->paint(canvas, s);
        canvas.pop_clip();
        canvas.restore();
        g.scroll().paint(canvas, s);
        current->paint_overlays(canvas, s);
        g.paint_veil(canvas, s);
        return g.pending_page || !g.veil.settled() || !g.theme.settled() || g.header_widgets.animating() || current->animating() || g.scroll().animating() ||
               (g.title_bar && g.title_bar->animating());
    }
    g.header_widgets.paint(canvas, s);
    canvas.save();
    canvas.push_clip(viewport);
    for (const Row& row : g.rows) {
        row.caption->paint(canvas, s);
    }
    for (std::size_t i = 0; i < res::all_icons.size(); ++i) {
        g.icon_font.draw(canvas, res::all_icons[i], g.symbol_spots[i], g.layout.symbol_size, s[g.layout.caption]);
    }
    g.chips.paint(canvas, s);
    g.chrome.paint(canvas, s);
    g.controls.paint(canvas, s);
    canvas.pop_clip();
    canvas.restore();
    g.scroll().paint(canvas, s);
    g.menu->paint(canvas, s);
    if (g.font_menu) {
        g.font_menu->paint(canvas, s);
    }
    g.dialog->paint(canvas, s);
    for (const auto& t : g.tooltips) {
        t->paint(canvas, s);
    }
    g.paint_veil(canvas, s);
    bool tooltips_moving = false;
    for (const auto& t : g.tooltips) {
        tooltips_moving = tooltips_moving || t->animating();
    }
    return g.pending_page || !g.veil.settled() || !g.theme.settled() || g.reflow.animating() || g.scroll().animating() || g.chips.animating() || g.controls.animating() || g.chrome.animating() || g.header_widgets.animating() || g.menu->animating() ||
           (g.font_menu && g.font_menu->animating()) || g.dialog->animating() || tooltips_moving || (g.title_bar && g.title_bar->animating());
}

std::optional<render::Rect> Gallery::caret_area() const {
    if (Page* p = impl_->current_page()) {
        return p->caret_area();
    }
    if (const auto caret = impl_->dialog->caret_area()) {
        return caret;
    }
    return impl_->focus.caret_area();
}

std::optional<double> Gallery::next_wake_seconds() const {
    std::optional<double> wake;
    auto merge = [&wake](std::optional<double> w) {
        if (w) {
            wake = wake ? std::min(*wake, *w) : *w;
        }
    };
    const double bar = impl_->page_box->scroll().next_wake_seconds();
    if (bar >= 0.0) {
        merge(bar);
    }
    if (Page* p = impl_->current_page()) {
        merge(p->next_wake_seconds());
        return wake;
    }
    merge(impl_->focus.next_wake_seconds());
    merge(impl_->dialog->next_wake_seconds());
    for (const auto& t : impl_->tooltips) {
        merge(t->next_wake_seconds());
    }
    return wake;
}

std::optional<input::Cursor> Gallery::cursor() const {
    if (Page* p = impl_->current_page()) {
        return p->cursor();
    }
    return std::nullopt;
}

std::vector<widgets::AccessibleNode> Gallery::accessible_nodes() const {
    std::vector<widgets::AccessibleNode> nodes = impl_->header_widgets.accessible_nodes();
    if (Page* p = impl_->current_page()) {
        for (widgets::AccessibleNode& n : p->accessible_nodes()) {
            nodes.push_back(std::move(n));
        }
        return nodes;
    }
    for (widgets::AccessibleNode& n : impl_->chrome.accessible_nodes()) {
        nodes.push_back(std::move(n));
    }
    for (widgets::AccessibleNode& n : impl_->controls.accessible_nodes()) {
        nodes.push_back(std::move(n));
    }
    if (auto m = impl_->menu->accessible()) {
        nodes.push_back(std::move(*m));
    }
    if (impl_->font_menu) {
        if (auto m = impl_->font_menu->accessible()) {
            nodes.push_back(std::move(*m));
        }
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

std::vector<widgets::AccessibleNode> Gallery::page_nodes() const {
    if (Page* p = impl_->current_page()) {
        return p->accessible_nodes();
    }
    std::vector<widgets::AccessibleNode> nodes = impl_->chrome.accessible_nodes();
    for (widgets::AccessibleNode& n : impl_->controls.accessible_nodes()) {
        nodes.push_back(std::move(n));
    }
    return nodes;
}

std::vector<render::Rect> Gallery::layout_blocks() const {
    if (Page* p = impl_->current_page()) {
        return p->layout_blocks();
    }
    return impl_->reflow.blocks();
}

render::Rect Gallery::page_area() const {
    return impl_->page_box->rect();
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
        for (auto& p : impl_->pages) {
            if (p->set_enabled(name, enabled)) {
                return true;
            }
        }
        return false;
    }
    it->second(enabled);
    return true;
}

void Gallery::set_typeface(text::Typeface typeface) {
    if (impl_->typefaces && impl_->typefaces->typeface != typeface) {
        impl_->pending_typeface = typeface;
    }
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
