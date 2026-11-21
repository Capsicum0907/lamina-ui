#include <memory>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/modal_navigation_rail.hpp"
#include "lamina/widgets/navigation_drawer.hpp"
#include "lamina/widgets/switch.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct DrawersLayout {
    layout::Size phone{412.0, 700.0};
    layout::Size tablet{520.0, 700.0};
    double column_space = 24.0;
    double bar_gap = 4.0;
    double bar_padding = 4.0;
};

const DrawersLayout drawers_layout;

class ChoiceNode final : public layout::Node {
public:
    ChoiceNode(layout::NodePtr first, layout::NodePtr second, const bool& use_second)
        : first_(std::move(first)), second_(std::move(second)), use_second_(use_second) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return (use_second_ ? second_ : first_)->measure(c); }
    void on_arrange(const render::Rect& r) override { (use_second_ ? second_ : first_)->arrange(r); }

private:
    layout::NodePtr first_;
    layout::NodePtr second_;
    const bool& use_second_;
};

class DrawersPage final : public BasePage {
public:
    explicit DrawersPage(PageContext context)
        : BasePage(std::move(context)), modal_frame_(drawers_layout.phone), push_frame_(drawers_layout.tablet), rail_frame_(drawers_layout.phone) {
        const DrawersLayout& l = drawers_layout;

        modal_ = std::make_unique<widgets::ModalNavigationDrawer>(widgets::NavigationDrawerOptions{{mail_entries(), 1}}, ctx.fonts);
        dismissible_body_ = screen_body("Inbox", "The dismissible drawer pushes this text aside. Drag it closed or press Escape.", dismissed_set_);
        dismissible_ = std::make_unique<widgets::DismissibleNavigationDrawer>(widgets::NavigationDrawerOptions{{mail_entries(), 1}},
                                                                                dismissible_body_, ctx.fonts);
        permanent_ = std::make_unique<widgets::DrawerSheet>(widgets::DrawerSheetOptions{mail_entries(), 1, widgets::DrawerSheetStyle::standard},
                                                            ctx.fonts);
        rail_menu_ = std::make_unique<widgets::IconButton>(widgets::IconButtonOptions{icon(res::icons::menu), "Expand rail"});
        rail_ = std::make_unique<widgets::ModalNavigationRail>(widgets::ModalNavigationRailOptions{rail_options(widgets::node(*rail_menu_))},
                                                               ctx.fonts);
        hiding_rail_ = std::make_unique<widgets::ModalNavigationRail>(
            widgets::ModalNavigationRailOptions{rail_options(nullptr), true}, ctx.fonts);

        auto& open_modal = bar_button(res::icons::menu, "Open navigation menu", [this] { open_modal_drawer(); });
        auto modal_screen = layout::column({app_bar(open_modal, "Inbox"),
                                            screen_body("", "Open the menu, or drag across this frame to the right.")});
        modal_screen->fill_cross(true);
        modal_frame_.on_arrange([this](const render::Rect& r) { modal_->set_area(r); });

        auto& toggle = bar_button(res::icons::menu, "Toggle navigation menu", [this] {
            if (dismissible_->target_value() == widgets::DrawerValue::open) {
                dismissible_->close();
            } else {
                dismissible_->open(source_);
            }
        });
        pin_ = &controls.emplace_with<widgets::Switch>(
            [this](widgets::Switch& s, bool clicked) {
                if (clicked) {
                    pinned_ = s.checked();
                    ctx.report(pinned_ ? "Drawer: permanent" : "Drawer: dismissible");
                }
            },
            widgets::SwitchOptions{.accessible_name = "Permanent"});
        focus_.add(*pin_);
        enabler("Permanent", *pin_);
        auto& pin_label =
            controls.emplace<widgets::Label>(widgets::LabelOptions{"Permanent", tokens::TypeRole::label_large, color::Role::on_surface_variant, false},
                                             ctx.fonts);
        stops_.push_back(std::make_unique<widgets::FocusStop>(widgets::forward_focus(*dismissible_, [this] { return !pinned_; })));
        focus_.add(*stops_.back());
        stops_.push_back(std::make_unique<widgets::FocusStop>(widgets::forward_focus(*permanent_, [this] { return pinned_; })));
        focus_.add(*stops_.back());
        auto pin_row = layout::row({widgets::node(pin_label), widgets::node(*pin_)});
        pin_row->spacing(demo_frame_style.gap).cross_align(layout::CrossAlign::center);
        auto bar = layout::row({widgets::node(toggle), title_node("Mail"), layout::weighted(layout::spacer({0.0, 0.0}), 1.0), pin_row});
        bar->spacing(l.bar_gap).cross_align(layout::CrossAlign::center).padding(layout::Insets::all(l.bar_padding));
        permanent_body_ = screen_body("Inbox", "The permanent drawer always stays beside the content.", permanent_set_);
        auto dismissible_node = widgets::measured_node(*dismissible_);
        auto permanent_node = widgets::permanent_drawer(*permanent_, permanent_body_);
        auto push_screen = layout::column({bar, layout::weighted(std::make_shared<ChoiceNode>(dismissible_node, permanent_node, pinned_), 1.0)});
        push_screen->fill_cross(true);

        auto& open_hiding = opener("Hiding rail", res::icons::menu_open, [this] {
            if (!hiding_open_) {
                focus_.suspend();
                hiding_open_ = true;
            }
            hiding_rail_->expand(source_);
        });
        auto rail_content = layout::column(
            {title_node("Home"), screen_body("", "The menu button in the rail expands it over a scrim. The rail below it hides when it collapses."),
             widgets::node(open_hiding)});
        rail_content->spacing(demo_frame_style.gap).padding(layout::Insets::all(demo_frame_style.padding));
        auto rail_screen = layout::row({widgets::measured_node(*rail_), layout::weighted(rail_content, 1.0)});
        rail_screen->cross_align(layout::CrossAlign::stretch);
        rail_frame_.on_arrange([this](const render::Rect& r) {
            rail_->set_area(r);
            hiding_rail_->set_area(r);
            hiding_rail_->arrange({r.x, r.y, 0.0, r.height});
        });
        focus_.add(*rail_menu_);
        focus_.add(*rail_);

        gated_tip(*rail_menu_, [] { return true; }, true);
        name_tip(open_modal);
        name_tip(toggle);

        auto columns = layout::row({heading("Modal navigation drawer", modal_frame_.node(modal_screen)),
                                    heading("Dismissible and permanent drawers", push_frame_.node(push_screen)),
                                    heading("Modal wide navigation rail", rail_frame_.node(rail_screen))});
        columns->spacing(l.column_space).cross_align(layout::CrossAlign::start);
        root = columns;
    }

    bool handle(const input::PointerEvent& e) override {
        source_ = input::InputSource::pointer;
        handle_tips(e);
        if (handle_dialog_tips(e)) {
            return true;
        }
        if (hiding_rail_->showing()) {
            report_rail(*hiding_rail_, hiding_rail_->handle(e));
            return true;
        }
        if (rail_->showing() && rail_menu_->handle(e)) {
            toggle_rail();
            return true;
        }
        const bool rail_was_showing = rail_->showing();
        const widgets::ModalRailResponse rr = rail_->handle(e);
        report_rail(*rail_, rr);
        if (rr.consumed && rail_was_showing) {
            return true;
        }
        const widgets::DrawerResponse md = modal_->handle(e);
        report_drawer(md, true);
        if (md.took_over) {
            BasePage::cancel_interaction();
        }
        if (md.consumed) {
            return true;
        }
        if (pinned_) {
            const widgets::DrawerResponse p = permanent_->handle(e);
            report_drawer(p, false);
        } else {
            const widgets::DrawerResponse d = dismissible_->handle(e);
            report_drawer(d, false);
            if (d.took_over) {
                BasePage::cancel_interaction();
            }
            if (d.consumed) {
                return true;
            }
        }
        if (!rail_->showing() && rail_menu_->handle(e)) {
            toggle_rail();
        }
        return handle_controls(e);
    }

    bool handle(const input::KeyEvent& e) override {
        source_ = input::InputSource::keyboard;
        handle_tips(e);
        handle_dialog_tips(e);
        if (hiding_rail_->showing()) {
            report_rail(*hiding_rail_, hiding_rail_->handle(e));
            return true;
        }
        if (rail_->showing()) {
            if (rail_menu_->handle(e)) {
                toggle_rail();
                return true;
            }
            report_rail(*rail_, rail_->handle(e));
            return true;
        }
        if (modal_->visible()) {
            report_drawer(modal_->handle(e), true);
            return true;
        }
        if (e.key != input::Key::tab) {
            if (rail_menu_->handle(e)) {
                toggle_rail();
                return true;
            }
            const widgets::ModalRailResponse rr = rail_->handle(e);
            report_rail(*rail_, rr);
            if (rr.consumed) {
                return true;
            }
            const widgets::DrawerResponse d = pinned_ ? permanent_->handle(e) : dismissible_->handle(e);
            report_drawer(d, false);
            if (d.consumed) {
                return true;
            }
        }
        return handle_controls(e);
    }

    bool handle(const input::WheelEvent& e) override {
        if (modal_->visible()) {
            return modal_->handle(e);
        }
        return pinned_ ? permanent_->handle(e) : dismissible_->handle(e);
    }

    void step(double dt) override {
        BasePage::step(dt);
        modal_->step(dt);
        dismissible_->step(dt);
        permanent_->step(dt);
        rail_->step(dt);
        hiding_rail_->step(dt);
        rail_menu_->step(dt);
        if (rail_->expanded() != rail_menu_open_) {
            rail_menu_open_ = rail_->expanded();
            rail_menu_->set_icon(icon(rail_menu_open_ ? res::icons::menu_open : res::icons::menu));
            rail_menu_->set_accessible_name(rail_menu_open_ ? "Collapse rail" : "Expand rail");
        }
        if (modal_open_ && !modal_->visible()) {
            modal_open_ = false;
            focus_.resume();
        }
        if (rail_open_ && !rail_->showing()) {
            rail_open_ = false;
            focus_.resume();
        }
        if (hiding_open_ && !hiding_rail_->showing()) {
            hiding_open_ = false;
            focus_.resume();
        }
    }

    bool animating() const override {
        return BasePage::animating() || modal_->animating() || dismissible_->animating() || permanent_->animating() || rail_->animating() ||
               hiding_rail_->animating() || rail_menu_->animating();
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const DemoFrame* f : {&modal_frame_, &push_frame_, &rail_frame_}) {
            f->paint_background(canvas, scheme);
        }
        BasePage::paint(canvas, scheme);
        push_frame_.begin_clip(canvas);
        (pinned_ ? permanent_set_ : dismissed_set_).paint(canvas, scheme);
        if (pinned_) {
            permanent_->paint(canvas, scheme);
        } else {
            dismissible_->paint(canvas, scheme);
        }
        push_frame_.end_clip(canvas);
        modal_frame_.begin_clip(canvas);
        modal_->paint(canvas, scheme);
        modal_frame_.end_clip(canvas);
        rail_frame_.begin_clip(canvas);
        rail_->paint(canvas, scheme);
        rail_menu_->paint(canvas, scheme);
        hiding_rail_->paint(canvas, scheme);
        rail_frame_.end_clip(canvas);
        for (const DemoFrame* f : {&modal_frame_, &push_frame_, &rail_frame_}) {
            f->paint_border(canvas, scheme);
        }
    }

    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint_overlays(canvas, scheme);
        paint_dialog_tips(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        if (pinned_) {
            nodes.push_back(permanent_->accessible());
        } else if (auto n = dismissible_->accessible()) {
            nodes.push_back(std::move(*n));
        }
        nodes.push_back(rail_menu_->accessible());
        for (const auto* r : {rail_.get(), hiding_rail_.get()}) {
            if (auto n = r->accessible()) {
                nodes.push_back(std::move(*n));
            }
        }
        if (auto n = modal_->accessible()) {
            nodes.push_back(std::move(*n));
        }
        for (const auto& t : dialog_tips) {
            if (auto n = t->accessible()) {
                nodes.push_back(std::move(*n));
            }
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        modal_->sheet().set_enabled(enabled);
        dismissible_->set_enabled(enabled);
        permanent_->set_enabled(enabled);
        rail_->set_enabled(enabled);
        hiding_rail_->set_enabled(enabled);
        rail_menu_->set_enabled(enabled);
        modal_->set_gestures_enabled(enabled);
        dismissible_->set_gestures_enabled(enabled);
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        modal_->cancel_interaction();
        dismissible_->cancel_interaction();
        permanent_->cancel_interaction();
        rail_->cancel_interaction();
        hiding_rail_->cancel_interaction();
        rail_menu_->cancel_interaction();
    }

private:
    std::vector<widgets::DrawerEntry> mail_entries() const {
        return {widgets::drawer_headline("Mail"),
                {widgets::DrawerEntryKind::item, "Inbox", icon(res::icons::inbox), widgets::BadgeSpec{"24", "24 new messages"}},
                {widgets::DrawerEntryKind::item, "Outbox", icon(res::icons::send)},
                {widgets::DrawerEntryKind::item, "Favorites", icon(res::icons::favorite), widgets::BadgeSpec{"3", "3 new messages"}},
                {widgets::DrawerEntryKind::item, "Trash", icon(res::icons::delete_)},
                widgets::drawer_divider(),
                widgets::drawer_headline("Labels"),
                {widgets::DrawerEntryKind::item, "Family", icon(res::icons::bookmark)},
                {widgets::DrawerEntryKind::item, "School", icon(res::icons::bookmark)},
                {widgets::DrawerEntryKind::item, "Archive", icon(res::icons::folder), std::nullopt, false}};
    }

    widgets::NavigationRailOptions rail_options(layout::NodePtr header) const {
        return {{{icon(res::icons::home), "Home"},
                 {icon(res::icons::star), "Starred"},
                 {icon(res::icons::chat), "Messages", {}, {}, true, {}, widgets::BadgeSpec{"5", "5 unread"}},
                 {icon(res::icons::settings), "Settings"}},
                0,
                false,
                std::move(header),
                "Main"};
    }

    widgets::IconButton& bar_button(icons::Symbol s, const char* name, std::function<void()> on_click) {
        auto& b = controls.emplace_with<widgets::IconButton>(
            [f = std::move(on_click)](widgets::IconButton&, bool clicked) {
                if (clicked) {
                    f();
                }
            },
            widgets::IconButtonOptions{icon(s), name});
        focus_.add(b);
        enabler(name, b);
        return b;
    }

    widgets::Button& opener(const char* label, icons::Symbol s, std::function<void()> on_click) {
        auto& b = controls.emplace_with<widgets::Button>(
            [f = std::move(on_click)](widgets::Button&, bool clicked) {
                if (clicked) {
                    f();
                }
            },
            widgets::ButtonOptions{label, tokens::ButtonStyle::tonal, tokens::ButtonSize::small, tokens::ButtonShape::round, icon(s)}, ctx.fonts);
        focus_.add(b);
        enabler(label, b);
        return b;
    }

    layout::NodePtr title_node(const char* text) { return title_node(text, controls); }

    layout::NodePtr title_node(const char* text, widgets::WidgetSet& set) {
        auto& t = set.emplace<widgets::Label>(widgets::LabelOptions{text, demo_frame_style.title_type, demo_frame_style.title, false}, ctx.fonts);
        return widgets::node(t);
    }

    layout::NodePtr app_bar(widgets::IconButton& button, const char* title) {
        auto bar = layout::row({widgets::node(button), title_node(title)});
        bar->spacing(drawers_layout.bar_gap).cross_align(layout::CrossAlign::center).padding(layout::Insets::all(drawers_layout.bar_padding));
        return bar;
    }

    layout::NodePtr screen_body(const char* title, const char* text) { return screen_body(title, text, controls); }

    layout::NodePtr screen_body(const char* title, const char* text, widgets::WidgetSet& set) {
        const DemoFrameStyle& s = demo_frame_style;
        std::vector<layout::NodePtr> nodes;
        if (title[0] != '\0') {
            nodes.push_back(title_node(title, set));
        }
        auto& b = set.emplace<widgets::Label>(widgets::LabelOptions{text, s.body_type, s.body, true}, ctx.fonts);
        nodes.push_back(widgets::node(b));
        auto c = layout::column(std::move(nodes));
        c->spacing(s.gap).padding(layout::Insets::all(s.padding));
        return c;
    }

    void open_modal_drawer() {
        if (!modal_open_) {
            focus_.suspend();
            modal_open_ = true;
        }
        modal_->open(source_);
    }

    void toggle_rail() {
        if (rail_->expanded()) {
            rail_->collapse();
            return;
        }
        if (!rail_open_) {
            const bool menu_focused = rail_menu_->focused();
            focus_.suspend();
            rail_open_ = true;
            rail_menu_->set_focused(menu_focused);
        }
        rail_->expand(source_);
        ctx.report("Rail: expanded");
    }

    void report_drawer(const widgets::DrawerResponse& r, bool modal) {
        if (r.selected) {
            const widgets::DrawerSheet& s = modal ? modal_->sheet() : pinned_ ? *permanent_ : dismissible_->sheet();
            ctx.report("Drawer: " + s.entry(*r.selected).label);
            if (modal) {
                modal_->close();
            }
        } else if (r.closed) {
            ctx.report("Drawer: closed");
        }
        if (modal && !modal_open_ && modal_->visible()) {
            focus_.suspend();
            modal_open_ = true;
        }
    }

    void report_rail(widgets::ModalNavigationRail& r, const widgets::ModalRailResponse& response) {
        if (response.selected) {
            ctx.report("Rail: " + std::string(r.rail().accessible().children[r.rail().selected()].name));
            if (r.hide_on_collapse()) {
                r.collapse();
            }
        } else if (response.collapsed) {
            ctx.report("Rail: collapsed");
        }
    }

    DemoFrame modal_frame_;
    DemoFrame push_frame_;
    widgets::WidgetSet dismissed_set_;
    widgets::WidgetSet permanent_set_;
    DemoFrame rail_frame_;
    layout::NodePtr dismissible_body_;
    layout::NodePtr permanent_body_;
    std::unique_ptr<widgets::ModalNavigationDrawer> modal_;
    std::unique_ptr<widgets::DismissibleNavigationDrawer> dismissible_;
    std::unique_ptr<widgets::DrawerSheet> permanent_;
    std::unique_ptr<widgets::IconButton> rail_menu_;
    std::unique_ptr<widgets::ModalNavigationRail> rail_;
    std::unique_ptr<widgets::ModalNavigationRail> hiding_rail_;
    std::vector<std::unique_ptr<widgets::FocusStop>> stops_;
    widgets::Switch* pin_ = nullptr;
    bool pinned_ = false;
    bool modal_open_ = false;
    bool rail_open_ = false;
    bool rail_menu_open_ = false;
    bool hiding_open_ = false;
    input::InputSource source_ = input::InputSource::pointer;
};

}

std::unique_ptr<Page> make_drawers_page(PageContext context) {
    return std::make_unique<DrawersPage>(std::move(context));
}

}
