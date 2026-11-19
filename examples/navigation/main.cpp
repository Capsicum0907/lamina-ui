#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using icons::Symbol;
using input::PointerAction;

namespace {

enum class Overlay { none, menu, grouped_menu, dialog, wrap_dialog };

struct Layout {
    double margin = 32.0;
    double row_gap = 24.0;
    double column_gap = 16.0;
    double label_width = 160.0;
    double rail_header_inset = 24.0;
    color::Role text = color::Role::on_surface;
    color::Role caption = color::Role::on_surface_variant;
    tokens::TypeRole header_type = tokens::TypeRole::headline_small;
    tokens::TypeRole caption_type = tokens::TypeRole::label_large;
    icons::IconStyle selected_icon{1.0};
};

layout::NodePtr captioned(widgets::Label& caption, const Layout& layout, layout::NodePtr content) {
    auto r = layout::row({layout::column({layout::spacer({layout.label_width, 0.0}), widgets::node(caption)}), layout::weighted(content, 1.0)});
    r->cross_align(layout::CrossAlign::center);
    r->self_align = layout::CrossAlign::stretch;
    return r;
}

std::string format_value(const char* name, double v) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%s: %.2f", name, v);
    return buffer;
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
        motion::Animated theme(1.0, tokens::motion_scheme.slow_effects());

        widgets::Label header("Navigation", layout.header_type, layout.text, fonts, false);
        widgets::Label status("—", layout.caption_type, layout.caption, fonts, false);
        widgets::IconButton dark_toggle({icon(Symbol::dark_mode), tokens::IconButtonStyle::outlined, tokens::IconButtonSize::small,
                                         tokens::ButtonShape::round, tokens::IconButtonWidth::uniform, true});
        auto report = [&status](std::string s) { status.set_text(std::move(s)); };

        widgets::TabRow primary_tabs({{{"Flights", icon(Symbol::send)}, {"Trips", icon(Symbol::bookmark)}, {"Explore", icon(Symbol::search)}},
                                      tokens::TabStyle::primary},
                                     fonts);
        widgets::TabRow secondary_tabs({{{"Overview"}, {"Specifications"}, {"レビュー"}, {"FAQ"}}, tokens::TabStyle::secondary},
                                       fonts);
        widgets::TabRow leading_tabs({{{"Mail", icon(Symbol::mail)}, {"People", icon(Symbol::person)}},
                                      tokens::TabStyle::primary,
                                      0,
                                      widgets::TabIconPosition::leading},
                                     fonts);
        widgets::Slider continuous({0.4});
        widgets::Slider stepped({60.0, 0.0, 100.0, 4});
        widgets::Slider* focused_slider = &continuous;

        widgets::Button menu_button({"Options", tokens::ButtonStyle::outlined, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                     icon(Symbol::more_vert)},
                                    fonts);
        widgets::Button dialog_button({"Delete draft", tokens::ButtonStyle::filled, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                       icon(Symbol::delete_)},
                                      fonts);
        const std::vector<std::string> menu_labels{"Edit", "Share", "Copy", "Settings"};
        widgets::Menu menu({{{menu_labels[0], icon(Symbol::edit)},
                             {menu_labels[1], icon(Symbol::share)},
                             {menu_labels[2], icon(Symbol::content_copy)},
                             {menu_labels[3], icon(Symbol::settings)}}},
                           fonts);
        widgets::Button grouped_button({"View", tokens::ButtonStyle::outlined, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                        icon(Symbol::view_list)},
                                       fonts);
        widgets::Button wrap_button({"Reset", tokens::ButtonStyle::tonal, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                     icon(Symbol::refresh)},
                                    fonts);
        widgets::GroupedMenu grouped_menu(
            {{{{{"List", icon(Symbol::view_list), std::nullopt, std::nullopt, true}, {"Grid", icon(Symbol::grid_view)}},
               "Layout",
               true,
               widgets::MenuSelection::single},
              {{{"Bookmarked", icon(Symbol::bookmark), std::nullopt, icon(Symbol::check)}, {"Starred", icon(Symbol::star)}},
               {},
               false,
               widgets::MenuSelection::multiple},
              {{{"Share", icon(Symbol::share)}, {"Settings", icon(Symbol::settings)}}}}},
            fonts);
        const std::vector<std::vector<std::string>> grouped_labels{{"List", "Grid"}, {"Bookmarked", "Starred"}, {"Share", "Settings"}};
        widgets::Dialog dialog({"Discard draft?", "This draft will be deleted from this device and cannot be restored.", "Discard", "Cancel",
                                icon(Symbol::delete_)},
                               fonts);
        widgets::Dialog wrap_dialog({"Reset all settings?", "Every setting returns to its default value. Your drafts are kept.",
                                     "Reset every setting to its default value", "Keep my current settings as they are"},
                                    fonts);

        const std::vector<std::string> sections{"Mail", "Starred", "Sent", "Contacts"};
        widgets::IconButton rail_open({icon(Symbol::menu)});
        widgets::IconButton rail_close({icon(Symbol::menu_open)});
        auto rail_toggle = std::make_shared<layout::Stack>(std::vector<layout::NodePtr>{widgets::node(rail_open), widgets::node(rail_close)});
        auto rail_header = layout::row({layout::spacer({layout.rail_header_inset, 0.0}), rail_toggle});
        widgets::NavigationRail rail({{{icon(Symbol::mail), sections[0], {}, layout.selected_icon},
                                       {icon(Symbol::star), sections[1], {}, layout.selected_icon},
                                       {icon(Symbol::send), sections[2], {}, layout.selected_icon},
                                       {icon(Symbol::person), sections[3], {}, layout.selected_icon}},
                                      0,
                                      false,
                                      rail_header},
                                     fonts);
        auto rail_button = [&rail, &rail_open, &rail_close]() -> widgets::IconButton& { return rail.expanded() ? rail_close : rail_open; };

        const std::vector<std::string> destinations{"Home", "Search", "Favorites", "Settings"};
        widgets::NavigationBar nav({{{icon(Symbol::home), destinations[0], {}, layout.selected_icon},
                                     {icon(Symbol::search), destinations[1], {}, layout.selected_icon},
                                     {icon(Symbol::favorite), destinations[2], {}, layout.selected_icon},
                                     {icon(Symbol::settings), destinations[3], {}, layout.selected_icon}}},
                                   fonts);

        widgets::Label primary_caption("Primary tabs", layout.caption_type, layout.caption, fonts, false);
        widgets::Label secondary_caption("Secondary tabs", layout.caption_type, layout.caption, fonts, false);
        widgets::Label leading_caption("Leading icons", layout.caption_type, layout.caption, fonts, false);
        widgets::Label continuous_caption(format_value("Slider", continuous.value()), layout.caption_type, layout.caption, fonts, false);
        widgets::Label stepped_caption(format_value("Steps", stepped.value()), layout.caption_type, layout.caption, fonts, false);
        widgets::Label overlay_caption("Menus and dialogs", layout.caption_type, layout.caption, fonts, false);

        auto header_row = layout::row({widgets::node(header), layout::flexible_space(), widgets::node(status), widgets::centered_node(dark_toggle)});
        header_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center).fill_main(true);
        header_row->self_align = layout::CrossAlign::stretch;
        auto overlay_row =
            layout::row({widgets::node(menu_button), widgets::node(grouped_button), widgets::node(dialog_button), widgets::node(wrap_button)});
        overlay_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center);

        auto page = layout::column({
            header_row,
            captioned(primary_caption, layout, widgets::node(primary_tabs)),
            captioned(secondary_caption, layout, widgets::node(secondary_tabs)),
            captioned(leading_caption, layout, widgets::node(leading_tabs)),
            captioned(continuous_caption, layout, widgets::node(continuous)),
            captioned(stepped_caption, layout, widgets::node(stepped)),
            captioned(overlay_caption, layout, overlay_row),
        });
        page->spacing(layout.row_gap).padding(layout::Insets::all(layout.margin));
        auto nav_node = widgets::node(nav);
        nav_node->self_align = layout::CrossAlign::stretch;
        auto main_column = layout::column({layout::weighted(page, 1.0), nav_node});
        main_column->fill_cross(true);
        page->self_align = layout::CrossAlign::stretch;
        auto root = layout::row({widgets::node(rail), layout::weighted(main_column, 1.0)});
        root->fill_cross(true);

        auto background = [&](const input::PointerEvent& e) {
            if (dark_toggle.handle(e)) {
                options.dark = dark_toggle.checked();
                from = color::mix(from, to, theme.value());
                to = color::spec_2021().build(options);
                theme.snap_to(0.0);
                theme.animate_to(1.0);
            }
            if (primary_tabs.handle(e)) {
                report("Primary tab: " + std::to_string(primary_tabs.selected() + 1));
            }
            if (secondary_tabs.handle(e)) {
                report("Secondary tab: " + std::to_string(secondary_tabs.selected() + 1));
            }
            if (leading_tabs.handle(e)) {
                report("Leading tab: " + std::to_string(leading_tabs.selected() + 1));
            }
            for (widgets::Slider* s : {&continuous, &stepped}) {
                if (s->handle(e)) {
                    focused_slider = s;
                    continuous_caption.set_text(format_value("Slider", continuous.value()));
                    stepped_caption.set_text(format_value("Steps", stepped.value()));
                }
                if (s->dragging()) {
                    focused_slider = s;
                }
            }
            if (nav.handle(e)) {
                report("Destination: " + destinations[nav.selected()]);
            }
            if (rail_button().handle(e)) {
                rail.set_expanded(!rail.expanded());
                rail_open.cancel_interaction();
                rail_close.cancel_interaction();
                report(rail.expanded() ? "Rail: expanded" : "Rail: collapsed");
            }
            if (rail.handle(e)) {
                report("Section: " + sections[rail.selected()]);
            }
            if (menu_button.handle(e)) {
                return Overlay::menu;
            }
            if (grouped_button.handle(e)) {
                return Overlay::grouped_menu;
            }
            if (dialog_button.handle(e)) {
                return Overlay::dialog;
            }
            if (wrap_button.handle(e)) {
                return Overlay::wrap_dialog;
            }
            return Overlay::none;
        };

        color::ColorScheme scheme = from;
        auto update = [&](const sdl3::Frame& frame) {
            const layout::Size window_size{frame.width_dp, frame.height_dp};
            rail.step(frame.dt_seconds);
            layout::run(*root, {0.0, 0.0, frame.width_dp, frame.height_dp});
            for (widgets::Dialog* d : {&dialog, &wrap_dialog}) {
                if (d->visible()) {
                    d->set_window(window_size);
                }
            }
            scheme = color::mix(from, to, theme.value());

            for (const auto& e : frame.pointer) {
                if (dialog.is_open()) {
                    const auto result = dialog.handle(e);
                    if (result == widgets::DialogResult::confirmed) {
                        report("Dialog: discarded");
                    } else if (result == widgets::DialogResult::dismissed) {
                        report("Dialog: cancelled");
                    }
                    continue;
                }
                if (wrap_dialog.is_open()) {
                    const auto result = wrap_dialog.handle(e);
                    if (result == widgets::DialogResult::confirmed) {
                        report("Dialog: settings reset");
                    } else if (result == widgets::DialogResult::dismissed) {
                        report("Dialog: settings kept");
                    }
                    continue;
                }
                if (grouped_menu.is_open()) {
                    const auto response = grouped_menu.handle(e);
                    if (response.chosen) {
                        const auto [g, i] = *response.chosen;
                        report("Menu: " + grouped_labels[g][i] + (grouped_menu.selected(g, i) ? " (on)" : ""));
                    }
                    if (response.consumed) {
                        continue;
                    }
                }
                if (menu.is_open()) {
                    const auto response = menu.handle(e);
                    if (response.chosen) {
                        report("Menu: " + menu_labels[*response.chosen]);
                    }
                    if (response.consumed) {
                        continue;
                    }
                }
                const Overlay opened = background(e);
                if (opened != Overlay::none) {
                    background({PointerAction::leave, 0.0, 0.0});
                }
                if (opened == Overlay::menu) {
                    menu.open(menu_button.bounds(), window_size);
                } else if (opened == Overlay::grouped_menu) {
                    grouped_menu.open(grouped_button.bounds(), window_size);
                } else if (opened == Overlay::dialog) {
                    dialog.open(window_size);
                } else if (opened == Overlay::wrap_dialog) {
                    wrap_dialog.open(window_size);
                }
            }
            for (const auto& k : frame.keys) {
                if (dialog.is_open()) {
                    if (dialog.handle(k) == widgets::DialogResult::dismissed) {
                        report("Dialog: cancelled");
                    }
                } else if (wrap_dialog.is_open()) {
                    if (wrap_dialog.handle(k) == widgets::DialogResult::dismissed) {
                        report("Dialog: settings kept");
                    }
                } else if (grouped_menu.is_open()) {
                    grouped_menu.handle(k);
                } else if (menu.is_open()) {
                    menu.handle(k);
                } else if (focused_slider->handle(k)) {
                    continuous_caption.set_text(format_value("Slider", continuous.value()));
                    stepped_caption.set_text(format_value("Steps", stepped.value()));
                }
            }
            return scheme[color::Role::surface];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            const double dt = frame.dt_seconds;
            rail.paint(canvas, scheme);
            for (widgets::Label* l : {&header, &status, &primary_caption, &secondary_caption, &leading_caption, &continuous_caption,
                                      &stepped_caption, &overlay_caption}) {
                l->paint(canvas, scheme);
            }
            theme.step(dt);
            dark_toggle.step(dt);
            primary_tabs.step(dt);
            secondary_tabs.step(dt);
            leading_tabs.step(dt);
            continuous.step(dt);
            stepped.step(dt);
            menu_button.step(dt);
            grouped_button.step(dt);
            dialog_button.step(dt);
            wrap_button.step(dt);
            rail_open.step(dt);
            rail_close.step(dt);
            grouped_menu.step(dt);
            wrap_dialog.step(dt);
            nav.step(dt);
            menu.step(dt);
            dialog.step(dt);
            dark_toggle.paint(canvas, scheme);
            primary_tabs.paint(canvas, scheme);
            secondary_tabs.paint(canvas, scheme);
            leading_tabs.paint(canvas, scheme);
            continuous.paint(canvas, scheme);
            stepped.paint(canvas, scheme);
            menu_button.paint(canvas, scheme);
            grouped_button.paint(canvas, scheme);
            dialog_button.paint(canvas, scheme);
            wrap_button.paint(canvas, scheme);
            rail_button().paint(canvas, scheme);
            nav.paint(canvas, scheme);
            menu.paint(canvas, scheme);
            grouped_menu.paint(canvas, scheme);
            dialog.paint(canvas, scheme);
            wrap_dialog.paint(canvas, scheme);
            return !theme.settled() || dark_toggle.animating() || primary_tabs.animating() || secondary_tabs.animating() ||
                   leading_tabs.animating() || continuous.animating() || stepped.animating() || menu_button.animating() ||
                   dialog_button.animating() || nav.animating() || menu.animating() || dialog.animating() ||
                   grouped_button.animating() || wrap_button.animating() || rail_open.animating() || rail_close.animating() ||
                   rail.animating() || grouped_menu.animating() || wrap_dialog.animating();
        };

        sdl3::run_app({"lamina-ui navigation", 1120, 860}, update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
