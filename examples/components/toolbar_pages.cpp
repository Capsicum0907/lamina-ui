#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/fab_menu.hpp"
#include "lamina/widgets/flexible_bottom_app_bar.hpp"
#include "lamina/widgets/floating_toolbar.hpp"
#include "lamina/widgets/list_view.hpp"
#include "res.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::components {

namespace {

namespace res = examples::res;
using widgets::FloatingToolbar;
using widgets::FloatingToolbarOptions;
using widgets::ToolbarAction;
using widgets::ToolbarSlot;

struct ToolbarsLayout {
    layout::Size phone{412.0, 668.0};
    double column_space = 24.0;
    std::size_t rows = 24;
    double side_offset = 16.0;
};

const ToolbarsLayout toolbars_layout;

class ToolbarsPage final : public BasePage {
public:
    explicit ToolbarsPage(PageContext context) : BasePage(std::move(context)) {
        const ToolbarsLayout& l = toolbars_layout;
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            frames_[i] = std::make_unique<Frame>(l.phone);
        }
        const std::array<const char*, 3> names{"Photos", "Notes", "Inbox"};
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            Frame& f = *frames_[i];
            std::vector<widgets::ListItemSpec> rows;
            for (std::size_t r = 0; r < l.rows; ++r) {
                widgets::ListItemSpec s{std::string(names[i]) + " " + std::to_string(r + 1), r % 2 == 0 ? "Supporting text" : ""};
                s.leading = widgets::ListSlot::of_avatar(std::string(1, names[i][0]));
                rows.push_back(std::move(s));
            }
            f.list = std::make_unique<widgets::ListView>(widgets::ListViewOptions{std::move(rows), false, std::string(names[i]) + " list"}, ctx.fonts);
        }

        FloatingToolbarOptions with_fab{{action(res::icons::person, "Account"), action(res::icons::edit, "Edit"), action(res::icons::favorite, "Favorite"),
                                         action(res::icons::more_vert, "More")}};
        with_fab.fab = widgets::ToolbarFab{icon(res::icons::add), "Add photo"};
        with_fab.accessible_name = "Photo tools";
        frames_[0]->toolbars.push_back(std::make_unique<FloatingToolbar>(std::move(with_fab), ctx.icon_font));

        FloatingToolbarOptions vertical{{action(res::icons::format_bold, "Bold"), action(res::icons::format_italic, "Italic"),
                                         action(res::icons::format_underlined, "Underline"), action(res::icons::palette, "Color")}};
        vertical.fab = widgets::ToolbarFab{icon(res::icons::check), "Done"};
        vertical.orientation = widgets::ToolbarOrientation::vertical;
        vertical.colors = widgets::ToolbarColorStyle::vibrant;
        vertical.accessible_name = "Format";
        frames_[1]->toolbars.push_back(std::make_unique<FloatingToolbar>(std::move(vertical), ctx.icon_font));
        FloatingToolbarOptions edges{{action(res::icons::undo, "Undo"), action(res::icons::redo, "Redo")}};
        edges.leading = {action(res::icons::text_fields, "Text"), action(res::icons::brush, "Brush")};
        edges.trailing = {action(res::icons::image, "Image"), action(res::icons::attach_file, "Attach")};
        edges.colors = widgets::ToolbarColorStyle::vibrant;
        edges.accessible_name = "Note tools";
        frames_[1]->toolbars.push_back(std::make_unique<FloatingToolbar>(std::move(edges), ctx.icon_font));

        frames_[2]->bar = std::make_unique<widgets::FlexibleBottomAppBar>(widgets::FlexibleBottomAppBarOptions{
            {action(res::icons::archive, "Archive"), action(res::icons::label, "Label"), action(res::icons::delete_, "Delete"),
             action(res::icons::mail, "Unread"), action(res::icons::more_vert, "More")},
            widgets::BottomBarArrangement::space_between, true, "Inbox actions"});
        frames_[2]->menu = std::make_unique<widgets::FabMenu>(
            widgets::FabMenuOptions{{{"Compose", icon(res::icons::edit)}, {"Chat", icon(res::icons::chat)}, {"Meeting", icon(res::icons::videocam)},
                                     {"Event", icon(res::icons::event)}},
                                    {icon(res::icons::add), icon(res::icons::close), "New"},
                                    widgets::FabMenuAlignment::end,
                                    "New item"},
            ctx.fonts);

        for (auto& f : frames_) {
            focus_.add(*f->list);
            for (auto& t : f->toolbars) {
                for (widgets::FocusStop* s : t->focus_stops()) {
                    focus_.add(*s);
                }
                if (t->has_fab()) {
                    FloatingToolbar* tp = t.get();
                    tip_anchors.push_back(std::make_unique<TipAnchor>(TipAnchor{[tp] { return *tp->fab_rect(); }, [tp] { return tp->fab_focused(); }}));
                    live_tip([tp] { return tp->accessible().children.back().name; }, *tip_anchors.back(), false);
                }
                for (ToolbarSlot slot : {ToolbarSlot::leading, ToolbarSlot::content, ToolbarSlot::trailing}) {
                    for (std::size_t i = 0; i < t->action_count(slot); ++i) {
                        FloatingToolbar* tp = t.get();
                        gated_tip(t->action(slot, i), [tp] { return tp->expanded(); });
                    }
                }
                f->expansion.push_back(widgets::ToolbarScrollExpansion(t->expanded()));
            }
            if (f->bar) {
                for (widgets::FocusStop* s : f->bar->focus_stops()) {
                    focus_.add(*s);
                }
                for (std::size_t i = 0; i < f->bar->size(); ++i) {
                    widgets::FlexibleBottomAppBar* bp = f->bar.get();
                    gated_tip(f->bar->action(i), [bp] { return bp->height() > 0.0; });
                }
            }
            if (f->menu) {
                for (widgets::FocusStop* s : f->menu->focus_stops()) {
                    focus_.add(*s);
                }
                name_tip(f->menu->button());
            }
        }

        std::vector<std::vector<layout::NodePtr>> sections;
        const std::array<const char*, 3> headings{"Floating toolbar with a FAB", "Vibrant, vertical and with side content",
                                                  "Flexible bottom app bar and FAB menu"};
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            Frame* f = frames_[i].get();
            f->frame.on_arrange([this, f](const render::Rect& r) {
                f->list->set_viewport(r);
                place(*f);
            });
            sections.push_back({heading(headings[i], f->frame.node(nullptr), layout::CrossAlign::stretch)});
        }
        root = columns(sections, l.column_space, l.column_space);
    }

    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        bool consumed = false;
        for (auto& fp : frames_) {
            Frame& f = *fp;
            bool used = false;
            if (f.menu) {
                const widgets::FabMenuResponse r = f.menu->handle(e);
                if (r.chosen) {
                    ctx.report("FAB menu: " + f.menu->item_name(*r.chosen));
                } else if (r.toggled) {
                    ctx.report(f.menu->expanded() ? "FAB menu: open" : "FAB menu: closed");
                }
                used = r.consumed;
            }
            for (auto& t : f.toolbars) {
                if (used) {
                    break;
                }
                const widgets::FloatingToolbarResponse r = t->handle(e);
                if (r.clicked) {
                    ctx.report("Toolbar: " + t->action(r.clicked->slot, r.clicked->index).accessible_name());
                }
                if (r.fab_clicked) {
                    ctx.report("Toolbar: FAB");
                }
                used = used || r.consumed;
            }
            if (f.bar && !used) {
                if (const auto i = f.bar->handle(e)) {
                    ctx.report("Bottom bar: " + f.bar->action(*i).accessible_name());
                    used = true;
                }
            }
            f.list->handle(used ? input::PointerEvent{input::PointerAction::leave, 0.0, 0.0} : e);
            consumed = consumed || used;
        }
        return handle_controls(e) || consumed;
    }
    bool holds_pointer() const override {
        return std::any_of(frames_.begin(), frames_.end(), [](const auto& f) { return f->list->holds_pointer(); }) || BasePage::holds_pointer();
    }


    bool handle(const input::KeyEvent& e) override {
        handle_tips(e);
        for (auto& f : frames_) {
            if (f->menu) {
                const widgets::FabMenuResponse r = f->menu->handle(e);
                if (r.chosen) {
                    ctx.report("FAB menu: " + f->menu->item_name(*r.chosen));
                }
                if (r.consumed) {
                    return true;
                }
            }
        }
        if (focus_.handle(e)) {
            return true;
        }
        for (auto& f : frames_) {
            for (auto& t : f->toolbars) {
                const widgets::FloatingToolbarResponse r = t->handle(e);
                if (r.clicked) {
                    ctx.report("Toolbar: " + t->action(r.clicked->slot, r.clicked->index).accessible_name());
                }
                if (r.fab_clicked) {
                    ctx.report("Toolbar: FAB");
                }
                if (r.consumed) {
                    return true;
                }
            }
            if (f->bar) {
                if (const auto i = f->bar->handle(e)) {
                    ctx.report("Bottom bar: " + f->bar->action(*i).accessible_name());
                    return true;
                }
            }
            if (f->list->handle(e).consumed) {
                return true;
            }
        }
        return controls.handle(e);
    }

    bool handle(const input::WheelEvent& e) override {
        bool used = false;
        for (auto& f : frames_) {
            used = f->list->handle(e) || used;
        }
        return used;
    }

    void step(double dt) override {
        BasePage::step(dt);
        for (auto& f : frames_) {
            f->list->step(dt);
            const double offset = f->list->scroll_area().offset();
            for (std::size_t i = 0; i < f->toolbars.size(); ++i) {
                if (const auto next = f->expansion[i].scrolled_to(offset)) {
                    f->toolbars[i]->set_expanded(*next);
                    ctx.report(*next ? "Toolbar: expanded" : "Toolbar: collapsed");
                }
                f->toolbars[i]->step(dt);
            }
            if (f->bar) {
                f->bar->follow(f->list->scroll_area());
                f->bar->step(dt);
            }
            if (f->menu) {
                f->menu->step(dt);
            }
            place(*f);
        }
    }

    bool animating() const override {
        bool any = BasePage::animating();
        for (const auto& f : frames_) {
            any = any || f->list->animating() || (f->bar && f->bar->animating()) || (f->menu && f->menu->animating());
            for (const auto& t : f->toolbars) {
                any = any || t->animating();
            }
        }
        return any;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const auto& f : frames_) {
            f->frame.paint_background(canvas, scheme);
        }
        BasePage::paint(canvas, scheme);
        for (const auto& f : frames_) {
            f->frame.begin_clip(canvas);
            f->list->paint(canvas, scheme);
            if (f->bar) {
                f->bar->paint(canvas, scheme);
            }
            for (const auto& t : f->toolbars) {
                t->paint(canvas, scheme);
            }
            if (f->menu) {
                f->menu->paint(canvas, scheme);
            }
            f->frame.end_clip(canvas);
            f->frame.paint_border(canvas, scheme);
        }
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const auto& f : frames_) {
            nodes.push_back(f->list->accessible());
            for (const auto& t : f->toolbars) {
                nodes.push_back(t->accessible());
            }
            if (f->bar) {
                nodes.push_back(f->bar->accessible());
            }
            if (f->menu) {
                nodes.push_back(f->menu->accessible());
            }
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        for (auto& f : frames_) {
            f->list->set_enabled(enabled);
            for (auto& t : f->toolbars) {
                t->set_enabled(enabled);
            }
            if (f->bar) {
                f->bar->set_enabled(enabled);
            }
        }
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        for (auto& f : frames_) {
            f->list->cancel_interaction();
            for (auto& t : f->toolbars) {
                t->cancel_interaction();
            }
            if (f->bar) {
                f->bar->cancel_interaction();
            }
            if (f->menu) {
                f->menu->cancel_interaction();
            }
        }
    }

    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        for (const auto& f : frames_) {
            const double w = f->list->next_wake_seconds();
            if (w >= 0.0) {
                wake = wake ? std::min(*wake, w) : w;
            }
        }
        return wake;
    }

private:
    struct Frame {
        explicit Frame(layout::Size size) : frame(size) {}
        DemoFrame frame;
        std::unique_ptr<widgets::ListView> list;
        std::vector<std::unique_ptr<FloatingToolbar>> toolbars;
        std::vector<widgets::ToolbarScrollExpansion> expansion;
        std::unique_ptr<widgets::FlexibleBottomAppBar> bar;
        std::unique_ptr<widgets::FabMenu> menu;
    };

    ToolbarAction action(icons::Symbol s, const char* name) const { return {icon(s), name}; }

    void place(Frame& f) {
        const render::Rect r = f.frame.rect();
        const double offset = current_style().floating_toolbar.screen_offset;
        for (auto& t : f.toolbars) {
            if (t->orientation() == widgets::ToolbarOrientation::vertical) {
                t->set_origin({r.x + r.width - offset - t->width(), r.y + (r.height - t->height()) / 2.0});
            } else {
                t->set_origin({r.x + (r.width - t->width()) / 2.0, r.y + r.height - offset - t->height()});
            }
        }
        double bottom = r.y + r.height;
        if (f.bar) {
            f.bar->set_width(r.width);
            f.bar->set_origin({r.x, 0.0});
            f.bar->set_bottom(r.y + r.height);
            bottom -= f.bar->height();
        }
        if (f.menu) {
            f.menu->set_origin({r.x + r.width - f.menu->width(), bottom - f.menu->height()});
        }
    }

    std::array<std::unique_ptr<Frame>, 3> frames_;
};

}

std::unique_ptr<Page> make_toolbars_page(PageContext context) {
    return std::make_unique<ToolbarsPage>(std::move(context));
}

}
