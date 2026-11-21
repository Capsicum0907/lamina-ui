#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/button_group.hpp"
#include "lamina/widgets/extended_fab.hpp"
#include "lamina/widgets/split_button.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

namespace res = examples::res;
using tokens::ButtonSize;
using tokens::ButtonStyle;
using widgets::ButtonGroup;
using widgets::ButtonGroupItem;
using widgets::ButtonGroupOptions;
using widgets::ButtonGroupSelection;
using widgets::SplitButton;

struct GroupsLayout {
    double column_space = 56.0;
    double row_gap = 20.0;
    double section_gap = 32.0;
    double overflow_width = 330.0;
    double label_width = 132.0;
};

const GroupsLayout groups_layout;

const char* size_name(ButtonSize s) {
    switch (s) {
    case ButtonSize::extra_small: return "Extra small";
    case ButtonSize::small: return "Small";
    case ButtonSize::medium: return "Medium";
    case ButtonSize::large: return "Large";
    case ButtonSize::extra_large: return "Extra large";
    case ButtonSize::count: break;
    }
    return "";
}

class GroupsPage final : public BasePage {
public:
    explicit GroupsPage(PageContext context) : BasePage(std::move(context)) {
        const GroupsLayout& l = groups_layout;
        auto left = layout::column({groups(), extended()});
        left->spacing(l.section_gap);
        auto right = layout::column({splits()});
        right->spacing(l.section_gap);
        auto columns = layout::row({left, right});
        columns->spacing(l.column_space).cross_align(layout::CrossAlign::start);
        root = columns;
    }

    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        if (Composite* open = open_composite()) {
            open->pointer(e);
            return true;
        }
        bool consumed = false;
        for (auto& c : composites_) {
            consumed = c.pointer(e) || consumed;
        }
        return handle_controls(e) || consumed;
    }

    bool handle(const input::KeyEvent& e) override {
        handle_tips(e);
        if (Composite* open = open_composite()) {
            open->key(e);
            return true;
        }
        for (auto& c : composites_) {
            if (c.key(e)) {
                return true;
            }
        }
        return handle_controls(e);
    }

    void step(double dt) override {
        BasePage::step(dt);
        for (auto& g : groups_) {
            g->step(dt);
        }
        for (auto& s : splits_) {
            s->step(dt);
        }
    }

    bool animating() const override {
        bool any = BasePage::animating();
        for (const auto& g : groups_) {
            any = any || g->animating();
        }
        for (const auto& s : splits_) {
            any = any || s->animating();
        }
        return any;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint(canvas, scheme);
        for (const auto& g : groups_) {
            g->paint(canvas, scheme);
        }
        for (const auto& s : splits_) {
            s->paint(canvas, scheme);
        }
    }

    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const auto& g : groups_) {
            g->paint_overlay(canvas, scheme);
        }
        for (const auto& s : splits_) {
            s->paint_overlay(canvas, scheme);
        }
        BasePage::paint_overlays(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const auto& g : groups_) {
            nodes.push_back(g->accessible());
        }
        for (const auto& s : splits_) {
            nodes.push_back(s->accessible());
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        for (std::size_t i = 0; i < groups_.size(); ++i) {
            groups_[i]->set_enabled(enabled && group_allowed_[i]);
        }
        for (auto& s : splits_) {
            s->set_enabled(enabled);
        }
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        for (auto& g : groups_) {
            g->cancel_interaction();
        }
        for (auto& s : splits_) {
            s->cancel_interaction();
        }
    }

    void set_window(layout::Size window) override {
        BasePage::set_window(window);
        for (auto& g : groups_) {
            g->set_window(window);
        }
        for (auto& s : splits_) {
            s->set_window(window);
        }
    }

private:
    struct Composite {
        std::function<bool(const input::PointerEvent&)> pointer;
        std::function<bool(const input::KeyEvent&)> key;
        std::function<bool()> menu_open;
    };

    Composite* open_composite() {
        for (auto& c : composites_) {
            if (c.menu_open()) {
                return &c;
            }
        }
        return nullptr;
    }

    layout::NodePtr captioned_at(const char* caption, layout::NodePtr content) {
        const auto style = tokens::text_style(tokens::type_style(base_page_layout.caption_type));
        captions.push_back(std::make_unique<widgets::Label>(
            widgets::LabelOptions{caption, base_page_layout.caption_type, base_page_layout.caption, false}, ctx.fonts));
        auto r = layout::row({examples::caption_cell(*captions.back(), groups_layout.label_width, style.line_height, style.line_height), content});
        r->cross_align(layout::CrossAlign::center);
        return r;
    }

    ButtonGroup& group(std::vector<layout::NodePtr>& rows, const char* caption, ButtonGroupOptions options, bool allowed = true,
                       double width = 0.0) {
        options.accessible_name = caption;
        groups_.push_back(std::make_unique<ButtonGroup>(std::move(options), ctx.fonts, ctx.icon_font));
        ButtonGroup& g = *groups_.back();
        group_allowed_.push_back(allowed);
        for (widgets::FocusStop* s : g.focus_stops()) {
            focus_.add(*s);
        }
        gated_tip(g.overflow_button(), [&g] { return g.overflowing(); });
        composites_.push_back(Composite{[this, &g](const input::PointerEvent& e) { return report(g, g.handle(e)); },
                                        [this, &g](const input::KeyEvent& e) { return report(g, g.handle(e)); },
                                        [&g] { return g.menu_open(); }});
        layout::NodePtr node = widgets::measured_node(g);
        if (width > 0.0) {
            node = width_limited(node, width);
        }
        rows.push_back(captioned_at(caption, node));
        enablers[caption] = [this, &g](bool e) { g.set_enabled(e); };
        if (!allowed) {
            g.set_enabled(false);
        }
        return g;
    }

    bool report(ButtonGroup& g, const widgets::ButtonGroupResponse& r) {
        if (r.clicked) {
            const std::size_t i = *r.clicked;
            std::string text = "Group: " + g.item_name(i);
            if (g.button(i).toggleable()) {
                text += g.checked(i) ? " on" : " off";
            }
            ctx.report(text);
        }
        return r.consumed;
    }

    ButtonGroupItem item(const char* label, std::optional<icons::Symbol> s = std::nullopt, bool checked = false) const {
        ButtonGroupItem it{label};
        if (s) {
            it.icon = icon(*s);
        }
        it.checked = checked;
        return it;
    }

    layout::NodePtr groups() {
        std::vector<layout::NodePtr> rows;
        group(rows, "Standard", {{item("Wi-Fi", res::icons::wifi, true), item("Bluetooth", res::icons::bluetooth), item("Location", res::icons::location_on)},
                                 ButtonGroupSelection::multiple});
        group(rows, "Actions", {{item("Cut", res::icons::content_cut), item("Copy", res::icons::content_copy), item("Paste", res::icons::content_paste)},
                                ButtonGroupSelection::none, false, ButtonStyle::tonal});
        group(rows, "Connected", {{item("Day"), item("Week", std::nullopt, true), item("Month"), item("Year")}, ButtonGroupSelection::single, true});
        group(rows, "Connected icons",
              {{item("Work", res::icons::work, true), item("Food", res::icons::restaurant), item("Coffee", res::icons::coffee),
                item("Home", res::icons::home)},
               ButtonGroupSelection::multiple, true, ButtonStyle::tonal});
        group(rows, "Overflow",
              {{item("Undo", res::icons::undo), item("Redo", res::icons::redo), item("Crop", res::icons::crop), item("Rotate", res::icons::rotate_right),
                item("Filter", res::icons::filter_list), item("Layers", res::icons::layers)},
               ButtonGroupSelection::none, false, ButtonStyle::outlined},
              true, groups_layout.overflow_width);
        group(rows, "Disabled", {{item("Left", res::icons::format_align_left), item("Center", res::icons::format_align_center, true),
                                  item("Right", res::icons::format_align_right)},
                                 ButtonGroupSelection::single, true},
              false);
        auto column = layout::column(std::move(rows));
        column->spacing(groups_layout.row_gap);
        return heading("Button groups", column);
    }

    SplitButton& split(std::vector<layout::NodePtr>& row, widgets::SplitButtonOptions options) {
        splits_.push_back(std::make_unique<SplitButton>(std::move(options), ctx.fonts, ctx.icon_font));
        SplitButton& s = *splits_.back();
        focus_.add(s.leading());
        focus_.add(s.trailing());
        name_tip(s.trailing());
        composites_.push_back(Composite{[this, &s](const input::PointerEvent& e) { return report(s, s.handle(e)); },
                                        [this, &s](const input::KeyEvent& e) { return report(s, s.handle(e)); },
                                        [&s] { return s.menu_open(); }});
        row.push_back(widgets::centered_node(s));
        return s;
    }

    bool report(SplitButton& s, const widgets::SplitButtonResponse& r) {
        const std::string name = "Split";
        if (r.leading_clicked) {
            ctx.report(name + ": " + s.leading().accessible_name());
        } else if (r.chosen && s.menu()) {
            ctx.report(name + ": item " + std::to_string(*r.chosen + 1));
        } else if (r.trailing_toggled) {
            ctx.report(name + (s.checked() ? ": menu open" : ": menu shut"));
        }
        return r.consumed;
    }

    std::vector<widgets::MenuItem> menu_items() const {
        return {{"Schedule send", icon(res::icons::event)}, {"Save draft", icon(res::icons::edit)}, {"Discard", icon(res::icons::delete_)}};
    }

    layout::NodePtr splits() {
        std::vector<layout::NodePtr> rows;
        for (std::size_t i = 0; i < static_cast<std::size_t>(ButtonSize::count); ++i) {
            const auto size = static_cast<ButtonSize>(i);
            std::vector<layout::NodePtr> row;
            widgets::SplitButtonOptions o{"Send", icon(res::icons::send)};
            o.size = size;
            o.trailing_name = "More send options";
            o.accessible_name = std::string(size_name(size)) + " split button";
            o.menu = menu_items();
            split(row, std::move(o));
            rows.push_back(captioned_at(size_name(size), items(std::move(row))));
        }
        std::vector<layout::NodePtr> styles;
        for (const auto& [style, name] : std::array<std::pair<ButtonStyle, const char*>, 3>{
                 {{ButtonStyle::tonal, "Tonal"}, {ButtonStyle::outlined, "Outlined"}, {ButtonStyle::elevated, "Elevated"}}}) {
            widgets::SplitButtonOptions o{name};
            o.style = style;
            o.trailing_name = std::string(name) + " options";
            o.accessible_name = std::string(name) + " split button";
            o.menu = menu_items();
            split(styles, std::move(o));
        }
        rows.push_back(captioned_at("Styles", items(std::move(styles))));
        auto column = layout::column(std::move(rows));
        column->spacing(groups_layout.row_gap);
        return heading("Split buttons", column);
    }

    layout::NodePtr extended() {
        std::vector<layout::NodePtr> fabs;
        std::vector<widgets::ExtendedFab*> made;
        for (const auto& [size, label] : std::array<std::pair<tokens::ExtendedFabSize, const char*>, 3>{
                 {{tokens::ExtendedFabSize::small, "Compose"}, {tokens::ExtendedFabSize::medium, "Navigate"}, {tokens::ExtendedFabSize::large, "Reply"}}}) {
            const icons::Symbol s = size == tokens::ExtendedFabSize::small ? res::icons::edit
                                    : size == tokens::ExtendedFabSize::medium ? res::icons::location_on
                                                                               : res::icons::send;
            auto& f = controls.emplace_with<widgets::ExtendedFab>(
                [this](widgets::ExtendedFab& w, bool clicked) {
                    if (clicked) {
                        ctx.report("FAB: " + w.accessible_name());
                    }
                },
                widgets::ExtendedFabOptions{label, icon(s), size}, ctx.fonts);
            focus_.add(f);
            fabs.push_back(widgets::centered_node(f));
            made.push_back(&f);
        }
        auto& toggle = controls.emplace_with<widgets::Button>(
            [this, made](widgets::Button& b, bool clicked) {
                if (clicked) {
                    for (widgets::ExtendedFab* f : made) {
                        f->set_expanded(!b.checked());
                    }
                    ctx.report(b.checked() ? "FABs: collapsed" : "FABs: expanded");
                }
            },
            [this] {
                widgets::ButtonOptions o{"Collapse", ButtonStyle::outlined, ButtonSize::small};
                o.toggle = widgets::ButtonToggle{};
                return o;
            }(),
            ctx.fonts);
        focus_.add(toggle);
        enabler("Collapse", toggle);
        fabs.push_back(widgets::centered_node(toggle));
        auto r = layout::row(std::move(fabs));
        r->spacing(groups_layout.row_gap).cross_align(layout::CrossAlign::center);
        return heading("Extended FABs", r);
    }

    std::vector<std::unique_ptr<ButtonGroup>> groups_;
    std::vector<bool> group_allowed_;
    std::vector<std::unique_ptr<SplitButton>> splits_;
    std::vector<Composite> composites_;
};

}

std::unique_ptr<Page> make_groups_page(PageContext context) {
    return std::make_unique<GroupsPage>(std::move(context));
}

}
