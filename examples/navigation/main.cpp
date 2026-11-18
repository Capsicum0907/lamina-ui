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
#include "lamina/platform/resources.hpp"
#include "lamina/platform/system_fonts.hpp"
#include "lamina/platform/window.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/navigation_bar.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/tabs.hpp"

using namespace lamina;
using icons::Symbol;
using platform::PointerAction;

namespace {

enum class Overlay { none, menu, dialog };

struct Layout {
    double margin = 32.0;
    double row_gap = 24.0;
    double column_gap = 16.0;
    double label_width = 160.0;
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
        const text::FontStack fonts = platform::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = platform::bundled_icon_font();
        auto icon = [&](Symbol s) { return icons::Icon{icon_font, s}; };

        color::SchemeOptions options = color::default_scheme_options;
        color::ColorScheme from = color::spec_2021().build(options);
        color::ColorScheme to = from;
        motion::Animated theme(1.0, tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::slow));

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
        widgets::Dialog dialog({"Discard draft?", "This draft will be deleted from this device and cannot be restored.", "Discard", "Cancel"},
                               fonts);

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
        widgets::Label overlay_caption("Menu and dialog", layout.caption_type, layout.caption, fonts, false);

        auto header_row = layout::row({widgets::node(header), layout::flexible_space(), widgets::node(status), widgets::centered_node(dark_toggle)});
        header_row->spacing(layout.column_gap).cross_align(layout::CrossAlign::center).fill_main(true);
        header_row->self_align = layout::CrossAlign::stretch;
        auto overlay_row = layout::row({widgets::node(menu_button), widgets::node(dialog_button)});
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
        auto root = layout::column({layout::weighted(page, 1.0), nav_node});
        root->fill_cross(true);
        page->self_align = layout::CrossAlign::stretch;

        auto background = [&](const platform::PointerEvent& e) {
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
            if (menu_button.handle(e)) {
                return Overlay::menu;
            }
            if (dialog_button.handle(e)) {
                return Overlay::dialog;
            }
            return Overlay::none;
        };

        platform::Window window({"lamina-ui navigation", 960, 820});
        platform::Frame frame;
        bool animating = true;
        while (window.next_frame(frame, animating)) {
            const layout::Size window_size{frame.width_dp, frame.height_dp};
            layout::run(*root, {0.0, 0.0, frame.width_dp, frame.height_dp});
            if (dialog.visible()) {
                dialog.set_window(window_size);
            }
            const color::ColorScheme scheme = color::mix(from, to, theme.value());

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
                } else if (opened == Overlay::dialog) {
                    dialog.open(window_size);
                }
            }
            for (const auto& k : frame.keys) {
                if (dialog.is_open()) {
                    if (dialog.handle(k) == widgets::DialogResult::dismissed) {
                        report("Dialog: cancelled");
                    }
                } else if (menu.is_open()) {
                    menu.handle(k);
                } else if (focused_slider->handle(k)) {
                    continuous_caption.set_text(format_value("Slider", continuous.value()));
                    stepped_caption.set_text(format_value("Steps", stepped.value()));
                }
            }

            auto& canvas = window.canvas();
            canvas.clear(scheme[color::Role::surface]);
            canvas.save();
            canvas.scale(frame.scale);
            const double dt = frame.dt_seconds;
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
            dialog_button.step(dt);
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
            dialog_button.paint(canvas, scheme);
            nav.paint(canvas, scheme);
            menu.paint(canvas, scheme);
            dialog.paint(canvas, scheme);
            animating = !theme.settled() || dark_toggle.animating() || primary_tabs.animating() || secondary_tabs.animating() ||
                        leading_tabs.animating() || continuous.animating() || stepped.animating() || menu_button.animating() ||
                        dialog_button.animating() || nav.animating() || menu.animating() || dialog.animating();
            canvas.restore();
            window.present();
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
