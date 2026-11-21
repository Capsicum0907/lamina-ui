#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/list_item.hpp"
#include "lamina/widgets/navigation_suite.hpp"
#include "lamina/widgets/pane_scaffold.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct AdaptiveLayout {
    double min_width = 360.0;
    double max_width = 1392.0;
    double initial_width = 1000.0;
    double frame_height = 620.0;
    double slider_width = 420.0;
    double control_gap = 24.0;
    double pane_padding = 16.0;
    double pane_gap = 12.0;
    std::array<double, 4> presets{412.0, 720.0, 1000.0, 1392.0};
    std::array<const char*, 4> preset_names{"Phone", "Tablet", "Laptop", "Monitor"};
    color::Role list_container = color::Role::surface_container_low;
    color::Role detail_container = color::Role::surface_container;
    tokens::CornerSize pane_corner = tokens::CornerSize::large;
    std::array<widgets::PaneExpansionAnchor, 4> anchors{widgets::PaneExpansionAnchor::proportion(0.0), widgets::PaneExpansionAnchor::from_start(300.0),
                                                        widgets::PaneExpansionAnchor::proportion(0.6), widgets::PaneExpansionAnchor::proportion(1.0)};
};

const AdaptiveLayout adaptive_layout;

struct Message {
    const char* sender;
    const char* subject;
    const char* body;
};

constexpr std::array<Message, 5> messages{{
    {"Ali Connors", "Brunch this weekend?", "I'll be in your neighborhood doing errands this weekend. Do you want to grab brunch?"},
    {"Sandra Adams", "Summer plans", "Wish I could come, but I'm out of town this weekend. Let's plan something for next month."},
    {"Trevor Hansen", "Oui oui", "Do you have Paris recommendations? Have you ever been? I'm planning a trip in the spring."},
    {"Britta Holt", "Birthday gift", "Have any ideas about what we should get Heidi for her birthday? She likes the outdoors."},
    {"Jennifer Scott", "Recipe to try", "We should eat this: grated squash, corn and tomatillo tacos. I found the recipe online."},
}};

class SizedFrame final : public layout::Node {
public:
    SizedFrame(DemoFrame& frame, layout::NodePtr content, const double& width, double height)
        : frame_(frame), content_(std::move(content)), width_(width), height_(height) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return c.constrain({width_, height_}); }
    void on_arrange(const render::Rect& r) override {
        frame_node_ = frame_node_ ? frame_node_ : frame_.node(content_);
        frame_.set_size({r.width, r.height});
        frame_node_->measure(layout::Constraints::tight({r.width, r.height}));
        frame_node_->arrange(r);
    }

private:
    DemoFrame& frame_;
    layout::NodePtr content_;
    layout::NodePtr frame_node_;
    const double& width_;
    double height_;
};

class Choice final : public layout::Node {
public:
    Choice(std::vector<layout::NodePtr> options, const std::size_t& current) : options_(std::move(options)), current_(current) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return options_[current_]->measure(c); }
    void on_arrange(const render::Rect& r) override { options_[current_]->arrange(r); }

private:
    std::vector<layout::NodePtr> options_;
    const std::size_t& current_;
};

struct PaneBody {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    layout::NodePtr node;
    widgets::ContentSlot slot() { return {node, &widgets, &focus}; }
};

class AdaptivePage final : public BasePage {
public:
    explicit AdaptivePage(PageContext context) : BasePage(std::move(context)), frame_({adaptive_layout.initial_width, adaptive_layout.frame_height}) {
        const AdaptiveLayout& l = adaptive_layout;
        width_ = l.initial_width;
        auto& width_label =
            controls.emplace<widgets::Label>(widgets::LabelOptions{"Window width", tokens::TypeRole::label_large, color::Role::on_surface_variant, false},
                                             ctx.fonts);
        slider_ = &controls.emplace_with<widgets::Slider>(
            [this](widgets::Slider& s, bool changed) {
                if (changed) {
                    set_width(std::round(s.value()));
                }
            },
            widgets::SliderOptions{.value = l.initial_width, .min = l.min_width, .max = l.max_width, .accessible_name = "Window width"});
        focus_.add(*slider_);
        enabler("Window width", *slider_);
        std::vector<widgets::Segment> presets;
        for (const char* name : l.preset_names) {
            presets.push_back({name});
        }
        presets_ = &controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow&, const widgets::SegmentedButtonResponse& r) {
                if (r.clicked) {
                    slider_->set_value(adaptive_layout.presets[*r.clicked]);
                    set_width(adaptive_layout.presets[*r.clicked]);
                }
            },
            widgets::SegmentedButtonRowOptions{presets, widgets::SegmentSelection::single, "Window presets"}, ctx.fonts, ctx.icon_font);
        focus_.add(*presets_);
        layouts_ = &controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow&, const widgets::SegmentedButtonResponse& r) {
                if (r.clicked) {
                    scaffold_choice_ = *r.clicked;
                    ctx.report(scaffold_choice_ == 0 ? "Panes: list-detail" : "Panes: supporting");
                }
            },
            widgets::SegmentedButtonRowOptions{{{"List-detail", std::nullopt, true}, {"Supporting"}}, widgets::SegmentSelection::single, "Pane layout"},
            ctx.fonts, ctx.icon_font);
        focus_.add(*layouts_);
        class_label_ = &controls.emplace<widgets::Label>(
            widgets::LabelOptions{"", tokens::TypeRole::title_small, color::Role::on_surface, false}, ctx.fonts);

        build_list_detail();
        build_supporting();
        auto content = std::make_shared<Choice>(std::vector<layout::NodePtr>{widgets::measured_node(*list_detail_), widgets::measured_node(*supporting_)},
                                                scaffold_choice_);
        suite_ = std::make_unique<widgets::NavigationSuiteScaffold>(
            widgets::NavigationSuiteScaffoldOptions{.items = {{icon(res::icons::inbox), "Inbox"},
                                                              {icon(res::icons::star), "Starred"},
                                                              {icon(res::icons::send), "Sent"},
                                                              {icon(res::icons::edit), "Drafts"}},
                                                    .content = content,
                                                    .accessible_name = "Mail"},
            ctx.fonts);
        focus_.add(*suite_);
        add_pane_focus();

        auto controls_row = layout::row({widgets::node(width_label), width_limited(widgets::node(*slider_), l.slider_width),
                                         widgets::node(*presets_), widgets::node(*layouts_), widgets::node(*class_label_)});
        controls_row->spacing(l.control_gap).cross_align(layout::CrossAlign::center);
        auto frame = std::make_shared<SizedFrame>(frame_, widgets::measured_node(*suite_), width_, l.frame_height);
        auto column = layout::column({controls_row, frame});
        column->spacing(base_page_layout.heading_gap * 2.0);
        root = column;
        set_width(l.initial_width);
    }

    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        const bool in_frame = e.action != input::PointerAction::leave && widgets::contains(frame_.rect(), e.x_dp, e.y_dp);
        const input::PointerEvent inside = in_frame ? e : input::PointerEvent{input::PointerAction::leave, 0.0, 0.0};
        const widgets::NavigationSuiteResponse s = suite_->handle(inside);
        if (s.selected) {
            ctx.report(std::string("Destination: ") + destination_name(*s.selected));
        }
        if (!s.consumed) {
            if (back_shown() && back_->handle(inside)) {
                go_back();
            }
            if (scaffold_choice_ == 0) {
                list_detail_->handle(inside);
            } else {
                if (comments_button_shown() && show_comments_->handle(inside)) {
                    show_comments();
                }
                supporting_->handle(inside);
            }
        }
        return handle_controls(e);
    }

    bool handle(const input::KeyEvent& e) override {
        handle_tips(e);
        if (focus_.handle(e)) {
            return true;
        }
        if (suite_->handle(e).consumed) {
            if (suite_->selected() != reported_destination_) {
                reported_destination_ = suite_->selected();
                ctx.report(std::string("Destination: ") + destination_name(reported_destination_));
            }
            return true;
        }
        if (back_shown() && back_->handle(e)) {
            go_back();
            return true;
        }
        if (scaffold_choice_ == 0) {
            const widgets::PaneScaffoldResponse r = list_detail_->handle(e);
            if (r.navigated_back) {
                ctx.report("Panes: back");
            }
            if (r.consumed) {
                return true;
            }
        } else {
            if (comments_button_shown() && show_comments_->handle(e)) {
                show_comments();
                return true;
            }
            const widgets::PaneScaffoldResponse r = supporting_->handle(e);
            if (r.navigated_back) {
                ctx.report("Panes: back");
            }
            if (r.consumed) {
                return true;
            }
        }
        return controls.handle(e);
    }

    bool handle(const input::WheelEvent&) override { return false; }

    void step(double dt) override {
        BasePage::step(dt);
        suite_->step(dt);
        list_detail_->step(dt);
        supporting_->step(dt);
        back_->step(dt);
        show_comments_->step(dt);
    }

    bool animating() const override {
        return BasePage::animating() || suite_->animating() || list_detail_->animating() || supporting_->animating() || back_->animating() ||
               show_comments_->animating();
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        frame_.paint_background(canvas, scheme);
        BasePage::paint(canvas, scheme);
        frame_.begin_clip(canvas);
        const widgets::ThreePaneScaffold& s = scaffold_choice_ == 0 ? *list_detail_ : *supporting_;
        paint_panes(canvas, scheme, s);
        s.paint(canvas, scheme);
        if (back_shown()) {
            back_->paint(canvas, scheme);
        }
        if (scaffold_choice_ == 1 && supporting_->pane_shown(widgets::supporting_pane::main) && !supporting_->pane_shown(widgets::supporting_pane::supporting)) {
            show_comments_->paint(canvas, scheme);
        }
        suite_->paint(canvas, scheme);
        frame_.end_clip(canvas);
        frame_.paint_border(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (widgets::AccessibleNode& n : suite_->accessible_nodes()) {
            nodes.push_back(std::move(n));
        }
        if (back_shown()) {
            nodes.push_back(back_->accessible());
        }
        const widgets::ThreePaneScaffold& s = scaffold_choice_ == 0 ? *list_detail_ : *supporting_;
        for (widgets::AccessibleNode& n : s.accessible_nodes()) {
            nodes.push_back(std::move(n));
        }
        if (scaffold_choice_ == 1 && comments_button_shown()) {
            nodes.push_back(show_comments_->accessible());
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        suite_->set_enabled(enabled);
        list_detail_->set_enabled(enabled);
        supporting_->set_enabled(enabled);
        back_->set_enabled(enabled);
        show_comments_->set_enabled(enabled);
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        suite_->cancel_interaction();
        list_detail_->cancel_interaction();
        supporting_->cancel_interaction();
        back_->cancel_interaction();
        show_comments_->cancel_interaction();
    }

    std::optional<input::Cursor> cursor() const override {
        return (scaffold_choice_ == 0 ? *list_detail_ : *supporting_).cursor();
    }

private:
    static const char* destination_name(std::size_t i) {
        constexpr std::array<const char*, 4> names{"Inbox", "Starred", "Sent", "Drafts"};
        return names[std::min(i, names.size() - 1)];
    }

    void set_width(double width) {
        width_ = std::clamp(width, adaptive_layout.min_width, adaptive_layout.max_width);
        const widgets::WindowAdaptiveInfo info{layout::compute_window_size_class(width_, adaptive_layout.frame_height)};
        suite_->update(info);
        const widgets::PaneScaffoldDirective d = widgets::calculate_pane_scaffold_directive(info);
        list_navigator_->set_directive(d);
        supporting_navigator_->set_directive(d);
        const layout::WindowWidthClass c = info.window_size_class.width_class();
        const std::string text = std::to_string(static_cast<int>(width_)) + " dp, " + std::string(layout::name(c));
        if (text != shown_class_) {
            class_label_->set_text(text);
            if (layout::name(c) != reported_class_) {
                reported_class_ = std::string(layout::name(c));
                ctx.report("Width: " + reported_class_);
            }
            shown_class_ = text;
        }
    }

    void build_list_detail() {
        const AdaptiveLayout& l = adaptive_layout;
        list_navigator_ = std::make_unique<widgets::ThreePaneScaffoldNavigator>(
            widgets::list_detail_pane_navigator(widgets::calculate_pane_scaffold_directive({layout::compute_window_size_class(l.initial_width, l.frame_height)})));
        std::vector<layout::NodePtr> rows;
        for (std::size_t i = 0; i < messages.size(); ++i) {
            auto& item = list_.widgets.emplace_with<widgets::ListItem>(
                [this, i](widgets::ListItem&, const widgets::ListItemResponse& r) {
                    if (r.clicked) {
                        open_message(i);
                    }
                },
                widgets::ListItemSpec{.headline = messages[i].sender,
                                      .supporting = messages[i].subject,
                                      .leading = widgets::ListSlot::of_avatar(std::string(1, messages[i].sender[0])),
                                      .selected = i == 0,
                                      .transparent = true},
                ctx.fonts);
            list_.focus.add(item);
            items_.push_back(&item);
            rows.push_back(widgets::node(item));
        }
        auto list_column = layout::column(std::move(rows));
        list_column->fill_cross(true).padding(layout::Insets::symmetric(0.0, l.pane_padding / 2.0));
        list_.node = list_column;

        back_ = std::make_unique<widgets::IconButton>(widgets::IconButtonOptions{icon(res::icons::arrow_back), "Back"});
        subject_ = &detail_.widgets.emplace<widgets::Label>(widgets::LabelOptions{messages[0].subject, tokens::TypeRole::headline_small}, ctx.fonts);
        sender_ = &detail_.widgets.emplace<widgets::Label>(
            widgets::LabelOptions{messages[0].sender, tokens::TypeRole::label_large, color::Role::on_surface_variant, false}, ctx.fonts);
        body_ = &detail_.widgets.emplace<widgets::Label>(
            widgets::LabelOptions{messages[0].body, tokens::TypeRole::body_large, color::Role::on_surface, true}, ctx.fonts);
        auto& reply = detail_.widgets.emplace<widgets::Button>(
            widgets::ButtonOptions{.label = "Reply", .style = tokens::ButtonStyle::tonal, .leading_icon = icon(res::icons::send)}, ctx.fonts);
        auto& archive = detail_.widgets.emplace<widgets::Button>(
            widgets::ButtonOptions{.label = "Archive", .style = tokens::ButtonStyle::text, .leading_icon = icon(res::icons::archive)}, ctx.fonts);
        detail_.focus.add(reply);
        detail_.focus.add(archive);
        auto back_slot = std::make_shared<layout::Leaf>(
            [this](const layout::Constraints&) { return back_shown() ? layout::Size{back_->width(), back_->height()} : layout::Size{0.0, 0.0}; },
            [this](const render::Rect& r) { back_->set_origin({r.x, r.y + (r.height - back_->height()) / 2.0}); });
        auto head = layout::row({back_slot, layout::weighted(widgets::node(*subject_), 1.0)});
        head->spacing(l.pane_gap).cross_align(layout::CrossAlign::center);
        auto actions = layout::row({widgets::node(reply), widgets::node(archive)});
        actions->spacing(l.pane_gap);
        auto detail_column = layout::column({head, widgets::node(*sender_), widgets::node(*body_), actions});
        detail_column->spacing(l.pane_gap).padding(layout::Insets::all(l.pane_padding)).fill_cross(true);
        detail_.node = detail_column;

        widgets::ThreePaneScaffoldOptions o;
        o.order = widgets::list_detail_pane_order;
        o.panes[static_cast<std::size_t>(widgets::list_detail_pane::list)] = list_.slot();
        o.panes[static_cast<std::size_t>(widgets::list_detail_pane::detail)] = detail_.slot();
        o.names[static_cast<std::size_t>(widgets::list_detail_pane::list)] = "Messages";
        o.names[static_cast<std::size_t>(widgets::list_detail_pane::detail)] = "Message";
        o.expansion_handle = true;
        o.anchors = {l.anchors.begin(), l.anchors.end()};
        o.animate_directive_changes = true;
        list_detail_ = std::make_unique<widgets::ThreePaneScaffold>(std::move(o), *list_navigator_);
    }

    void build_supporting() {
        const AdaptiveLayout& l = adaptive_layout;
        supporting_navigator_ = std::make_unique<widgets::ThreePaneScaffoldNavigator>(
            widgets::supporting_pane_navigator(widgets::calculate_pane_scaffold_directive({layout::compute_window_size_class(l.initial_width, l.frame_height)})));
        auto& title = main_.widgets.emplace<widgets::Label>(widgets::LabelOptions{"Trip to Paris", tokens::TypeRole::headline_small}, ctx.fonts);
        auto& text = main_.widgets.emplace<widgets::Label>(
            widgets::LabelOptions{"Day one: the Louvre in the morning, lunch by the river and a walk through the Marais. Day two: Montmartre and "
                                  "the Musee d'Orsay. The supporting pane beside this document holds its comments; on a compact window it "
                                  "moves below the document or behind a button.",
                                  tokens::TypeRole::body_large, color::Role::on_surface, true},
            ctx.fonts);
        show_comments_ = std::make_unique<widgets::Button>(
            widgets::ButtonOptions{.label = "Show comments", .style = tokens::ButtonStyle::tonal, .leading_icon = icon(res::icons::chat)}, ctx.fonts);
        auto comments_slot = std::make_shared<layout::Leaf>(
            [this](const layout::Constraints&) {
                return comments_button_shown() ? layout::Size{show_comments_->width(), show_comments_->height()} : layout::Size{0.0, 0.0};
            },
            [this](const render::Rect& r) { show_comments_->set_origin({r.x, r.y}); });
        auto main_column = layout::column({widgets::node(title), widgets::node(text), comments_slot});
        main_column->spacing(l.pane_gap).padding(layout::Insets::all(l.pane_padding)).fill_cross(true);
        main_.node = main_column;
        std::vector<layout::NodePtr> rows;
        constexpr std::array<std::array<const char*, 2>, 3> comments{
            {{"Sandra Adams", "Book the museum tickets early."}, {"Ali Connors", "The river cruise is lovely at night."}, {"Trevor Hansen", "Add a bakery stop!"}}};
        for (const auto& c : comments) {
            auto& item = support_.widgets.emplace<widgets::ListItem>(
                widgets::ListItemSpec{.headline = c[0], .supporting = c[1], .leading = widgets::ListSlot::of_avatar(std::string(1, c[0][0])),
                                      .transparent = true},
                ctx.fonts);
            support_.focus.add(item);
            rows.push_back(widgets::node(item));
        }
        auto support_column = layout::column(std::move(rows));
        support_column->fill_cross(true).padding(layout::Insets::symmetric(0.0, l.pane_padding / 2.0));
        support_.node = support_column;
        widgets::ThreePaneScaffoldOptions o;
        o.order = widgets::supporting_pane_order;
        o.panes[static_cast<std::size_t>(widgets::supporting_pane::main)] = main_.slot();
        o.panes[static_cast<std::size_t>(widgets::supporting_pane::supporting)] = support_.slot();
        o.names[static_cast<std::size_t>(widgets::supporting_pane::main)] = "Document";
        o.names[static_cast<std::size_t>(widgets::supporting_pane::supporting)] = "Comments";
        o.animate_directive_changes = true;
        supporting_ = std::make_unique<widgets::ThreePaneScaffold>(std::move(o), *supporting_navigator_);
    }

    void add_pane_focus() {
        namespace ld = widgets::list_detail_pane;
        namespace sp = widgets::supporting_pane;
        stops_.push_back(std::make_unique<widgets::FocusStop>(widgets::forward_focus(*back_, [this] { return back_shown(); })));
        focus_.add(*stops_.back());
        focus_.add_group(list_.focus, [this] { return scaffold_choice_ == 0 && list_detail_->pane_shown(ld::list); });
        stops_.push_back(std::make_unique<widgets::FocusStop>(widgets::FocusStop::Hooks{
            [this] { return list_detail_->focused(); }, [this](bool f) { list_detail_->set_focused(f); },
            [this] { return scaffold_choice_ == 0 && list_detail_->can_focus(); }, [this] { return list_detail_->bounds_for_focus(); }}));
        focus_.add(*stops_.back());
        focus_.add_group(detail_.focus, [this] { return scaffold_choice_ == 0 && list_detail_->pane_shown(ld::detail); });
        focus_.add_group(main_.focus, [this] { return scaffold_choice_ == 1 && supporting_->pane_shown(sp::main); });
        stops_.push_back(std::make_unique<widgets::FocusStop>(widgets::forward_focus(*show_comments_, [this] { return scaffold_choice_ == 1 && comments_button_shown(); })));
        focus_.add(*stops_.back());
        focus_.add_group(support_.focus, [this] { return scaffold_choice_ == 1 && supporting_->pane_shown(sp::supporting); });
    }

    bool back_shown() const {
        namespace ld = widgets::list_detail_pane;
        return scaffold_choice_ == 0 && list_detail_->pane_shown(ld::detail) && !list_detail_->pane_shown(ld::list) &&
               list_navigator_->can_navigate_back();
    }

    bool comments_button_shown() const {
        namespace sp = widgets::supporting_pane;
        return !supporting_->pane_shown(sp::supporting) && !supporting_->pane_visible(sp::supporting);
    }

    void go_back() {
        if (list_detail_->back()) {
            ctx.report("Panes: back");
        }
    }

    void show_comments() {
        supporting_navigator_->navigate_to(widgets::supporting_pane::supporting);
        ctx.report("Panes: comments");
    }

    void open_message(std::size_t i) {
        for (std::size_t k = 0; k < items_.size(); ++k) {
            items_[k]->set_selected(k == i);
        }
        subject_->set_text(messages[i].subject);
        sender_->set_text(messages[i].sender);
        body_->set_text(messages[i].body);
        list_navigator_->navigate_to(widgets::list_detail_pane::detail, i);
        ctx.report(std::string("Open: ") + std::string(messages[i].sender).substr(0, std::string(messages[i].sender).find(' ')));
    }

    void paint_panes(render::Canvas& canvas, const color::ColorScheme& scheme, const widgets::ThreePaneScaffold& s) const {
        const AdaptiveLayout& l = adaptive_layout;
        for (std::size_t k = 0; k < widgets::pane_count; ++k) {
            const auto role = static_cast<widgets::PaneRole>(k);
            if (!s.pane_visible(role)) {
                continue;
            }
            const render::Rect r = s.pane_rect(role);
            if (r.width <= 0.0) {
                continue;
            }
            const bool primary = role == widgets::PaneRole::primary;
            const double radius = tokens::corner_radius(l.pane_corner, r.width, r.height);
            canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(radius)), scheme[primary ? l.detail_container : l.list_container]);
        }
    }

    DemoFrame frame_;
    double width_ = 0.0;
    widgets::Slider* slider_ = nullptr;
    widgets::SegmentedButtonRow* presets_ = nullptr;
    widgets::SegmentedButtonRow* layouts_ = nullptr;
    widgets::Label* class_label_ = nullptr;
    std::string shown_class_;
    std::string reported_class_;
    std::size_t scaffold_choice_ = 0;
    std::size_t reported_destination_ = 0;
    std::unique_ptr<widgets::NavigationSuiteScaffold> suite_;
    PaneBody list_;
    PaneBody detail_;
    PaneBody main_;
    PaneBody support_;
    std::vector<widgets::ListItem*> items_;
    widgets::Label* subject_ = nullptr;
    widgets::Label* sender_ = nullptr;
    widgets::Label* body_ = nullptr;
    std::unique_ptr<widgets::IconButton> back_;
    std::unique_ptr<widgets::Button> show_comments_;
    std::unique_ptr<widgets::ThreePaneScaffoldNavigator> list_navigator_;
    std::unique_ptr<widgets::ThreePaneScaffoldNavigator> supporting_navigator_;
    std::unique_ptr<widgets::ThreePaneScaffold> list_detail_;
    std::unique_ptr<widgets::ThreePaneScaffold> supporting_;
    std::vector<std::unique_ptr<widgets::FocusStop>> stops_;
};

}

std::unique_ptr<Page> make_adaptive_page(PageContext context) {
    return std::make_unique<AdaptivePage>(std::move(context));
}

}
