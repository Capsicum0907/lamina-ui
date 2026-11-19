#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "common/canvas.hpp"
#include "common/theme.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/overlay_stack.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/tabs.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using icons::Symbol;

namespace {

enum class Opened { none, menu, grouped_menu, dialog, wrap_dialog };

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
        examples::ThemeFade theme(color::spec_2021().build(options));

        widgets::Label header({"Navigation", layout.header_type, layout.text, false}, fonts);
        widgets::Label status({"—", layout.caption_type, layout.caption, false}, fonts);
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

        widgets::Label primary_caption({"Primary tabs", layout.caption_type, layout.caption, false}, fonts);
        widgets::Label secondary_caption({"Secondary tabs", layout.caption_type, layout.caption, false}, fonts);
        widgets::Label leading_caption({"Leading icons", layout.caption_type, layout.caption, false}, fonts);
        widgets::Label continuous_caption({format_value("Slider", continuous.value()), layout.caption_type, layout.caption, false}, fonts);
        widgets::Label stepped_caption({format_value("Steps", stepped.value()), layout.caption_type, layout.caption, false}, fonts);
        widgets::Label overlay_caption({"Menus and dialogs", layout.caption_type, layout.caption, false}, fonts);

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

        widgets::WidgetSet page_controls;
        page_controls.add(dark_toggle);
        for (widgets::TabRow* t : {&primary_tabs, &secondary_tabs, &leading_tabs}) {
            page_controls.add(*t);
        }
        page_controls.add(continuous);
        page_controls.add(stepped);
        for (widgets::Button* b : {&menu_button, &grouped_button, &dialog_button, &wrap_button}) {
            page_controls.add(*b);
        }
        widgets::WidgetSet front;
        front.add(nav);
        front.add(menu);
        front.add(grouped_menu);
        front.add(dialog);
        front.add(wrap_dialog);

        widgets::OverlayStack overlays;
        overlays.add_base(page_controls, nav, rail, rail_open, rail_close);
        auto on_menu = [&](const widgets::MenuResponse& r) {
            if (r.chosen) {
                report("Menu: " + menu_labels[*r.chosen]);
            }
        };
        auto on_grouped_menu = [&](const widgets::GroupedMenuResponse& r) {
            if (r.chosen) {
                const auto [g, i] = *r.chosen;
                report("Menu: " + grouped_labels[g][i] + (grouped_menu.selected(g, i) ? " (on)" : ""));
            }
        };
        auto on_dialog = [&](widgets::DialogResult r) {
            if (r == widgets::DialogResult::confirmed) {
                report("Dialog: discarded");
            } else if (r == widgets::DialogResult::dismissed) {
                report("Dialog: cancelled");
            }
        };
        auto on_wrap_dialog = [&](widgets::DialogResult r) {
            if (r == widgets::DialogResult::confirmed) {
                report("Dialog: settings reset");
            } else if (r == widgets::DialogResult::dismissed) {
                report("Dialog: settings kept");
            }
        };

        auto background = [&](const input::PointerEvent& e) {
            if (dark_toggle.handle(e)) {
                options.dark = dark_toggle.checked();
                theme.fade_to(color::spec_2021().build(options));
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
                return Opened::menu;
            }
            if (grouped_button.handle(e)) {
                return Opened::grouped_menu;
            }
            if (dialog_button.handle(e)) {
                return Opened::dialog;
            }
            if (wrap_button.handle(e)) {
                return Opened::wrap_dialog;
            }
            return Opened::none;
        };

        color::ColorScheme scheme = theme.current();
        auto update = [&](const sdl3::Frame& frame) {
            const layout::Size window_size{frame.width_dp, frame.height_dp};
            rail.step(frame.dt_seconds);
            layout::run(*root, {0.0, 0.0, frame.width_dp, frame.height_dp});
            for (widgets::Dialog* d : {&dialog, &wrap_dialog}) {
                if (d->visible()) {
                    d->set_window(window_size);
                }
            }
            scheme = theme.current();

            for (const auto& e : frame.pointer) {
                if (overlays.handle(e)) {
                    continue;
                }
                const Opened opened = background(e);
                if (opened == Opened::menu) {
                    menu.open(menu_button.bounds(), window_size);
                    overlays.push(menu, on_menu);
                } else if (opened == Opened::grouped_menu) {
                    grouped_menu.open(grouped_button.bounds(), window_size);
                    overlays.push(grouped_menu, on_grouped_menu);
                } else if (opened == Opened::dialog) {
                    dialog.open(window_size);
                    overlays.push(dialog, on_dialog);
                } else if (opened == Opened::wrap_dialog) {
                    wrap_dialog.open(window_size);
                    overlays.push(wrap_dialog, on_wrap_dialog);
                }
            }
            for (const auto& k : frame.keys) {
                if (overlays.handle(k)) {
                    continue;
                }
                if (focused_slider->handle(k)) {
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
            page_controls.step(dt);
            rail_open.step(dt);
            rail_close.step(dt);
            front.step(dt);
            page_controls.paint(canvas, scheme);
            rail_button().paint(canvas, scheme);
            front.paint(canvas, scheme);
            return !theme.settled() || page_controls.animating() || rail_open.animating() || rail_close.animating() ||
                   rail.animating() || front.animating();
        };

        sdl3::run_app({"lamina-ui navigation", 1120, 860}, examples::window_canvas(), update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
