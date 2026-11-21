#include <array>
#include <memory>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/bottom_sheet.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/side_sheet.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct SheetsLayout {
    layout::Size phone{412.0, 700.0};
    layout::Size tablet{520.0, 700.0};
    double column_space = 24.0;
    double button_gap = 8.0;
    double standard_peek = 128.0;
};

const SheetsLayout sheets_layout;

const char* value_name(widgets::SheetValue v) {
    switch (v) {
    case widgets::SheetValue::hidden: return "hidden";
    case widgets::SheetValue::partially_expanded: return "partial";
    case widgets::SheetValue::expanded: return "expanded";
    }
    return "";
}

class SheetsPage final : public BasePage {
public:
    explicit SheetsPage(PageContext context)
        : BasePage(std::move(context)),
          modal_frame_(sheets_layout.phone),
          standard_frame_(sheets_layout.phone),
          side_frame_(sheets_layout.tablet) {
        const SheetsLayout& l = sheets_layout;

        modal_body_ = list_body(
            {action_item("Share", res::icons::share), action_item("Get link", res::icons::content_copy), action_item("Edit name", res::icons::edit),
             action_item("Add to album", res::icons::add), action_item("Bookmark", res::icons::bookmark), action_item("Download", res::icons::download),
             action_item("Send to a phone", res::icons::send), action_item("Mail", res::icons::mail), action_item("Details", res::icons::info),
             action_item("Move to folder", res::icons::folder), action_item("Favorite", res::icons::favorite), action_item("Delete", res::icons::delete_)},
            ctx.fonts);
        modal_sheet_ = std::make_unique<widgets::BottomSheet>(widgets::BottomSheetOptions{.content = modal_body_->slot()});

        standard_body_ = list_body({place_item("Fushimi Inari", "Shrine, 10 min walk"), place_item("Kiyomizu-dera", "Temple, 25 min by bus"),
                                    place_item("Nishiki Market", "Market, 12 min walk"), place_item("Arashiyama", "Bamboo grove, 30 min by train"),
                                    place_item("Gion", "District, 15 min walk")},
                                   ctx.fonts);
        standard_sheet_ = std::make_unique<widgets::BottomSheet>(widgets::BottomSheetOptions{.kind = widgets::BottomSheetKind::standard,
                                                                                             .content = standard_body_->slot(false),
                                                                                             .peek_height = l.standard_peek,
                                                                                             .initial = widgets::SheetValue::partially_expanded,
                                                                                             .accessible_name = "Nearby places"});
        focus_.add(*standard_sheet_->drag_handle());
        for (auto* item : standard_body_->items) {
            focus_.add(*item);
        }

        side_bodies_[0] = list_body(filter_items(), ctx.fonts);
        side_bodies_[1] = list_body(filter_items(), ctx.fonts);
        side_bodies_[2] = list_body(filter_items(), ctx.fonts);
        modal_side_ = std::make_unique<widgets::SideSheet>(side_options(widgets::SideSheetKind::modal, false, *side_bodies_[0]), ctx.fonts);
        detached_side_ = std::make_unique<widgets::SideSheet>(side_options(widgets::SideSheetKind::modal, true, *side_bodies_[1]), ctx.fonts);
        standard_side_ = std::make_unique<widgets::SideSheet>(side_options(widgets::SideSheetKind::standard, false, *side_bodies_[2]), ctx.fonts);

        auto& show = opener("Show bottom sheet", res::icons::expand_less, [this] { open_modal_sheet(); });
        auto modal_screen = screen("Photos", "Tap a photo's menu to see what you can do with it. The sheet opens to half its height.", {&show});
        modal_frame_.on_arrange([this](const render::Rect& r) { modal_sheet_->set_area(r); });

        auto standard_screen = screen("Kyoto", "Drag the sheet up to see every place nearby. It never hides; it peeks 128 dp.", {});
        standard_frame_.on_arrange([this](const render::Rect& r) { standard_sheet_->set_area(r); });

        auto& open_modal_side = opener("Modal side sheet", res::icons::menu_open, [this] { open_side(*modal_side_); });
        auto& open_detached = opener("Detached side sheet", res::icons::menu_open, [this] { open_side(*detached_side_); });
        auto& toggle_standard = opener("Standard side sheet", res::icons::view_list, [this] {
            if (standard_side_->target_value() == widgets::SideSheetValue::expanded) {
                standard_side_->close();
            } else {
                standard_side_->open(source_);
            }
        });
        auto side_screen = screen("Inventory", "Filters open in a side sheet. The standard one takes room beside the list.",
                                  {&open_modal_side, &open_detached, &toggle_standard});
        standard_side_->add_header_stops(focus_);
        for (auto* item : side_bodies_[2]->items) {
            focus_.add(*item);
        }
        standard_side_->add_action_stops(focus_);
        auto side_row = layout::row({layout::weighted(side_screen, 1.0), widgets::measured_node(*standard_side_)});
        side_row->cross_align(layout::CrossAlign::stretch);
        side_frame_.on_arrange([this](const render::Rect& r) {
            modal_side_->set_area(r);
            detached_side_->set_area(r);
        });

        gated_tip(*modal_sheet_->drag_handle(), [this] { return modal_sheet_->visible(); }, true);
        gated_tip(*standard_sheet_->drag_handle(), [] { return true; }, true);
        for (widgets::SideSheet* s : {modal_side_.get(), detached_side_.get(), standard_side_.get()}) {
            gated_tip(*s->close_button(), [s] { return s->visible(); }, true);
            gated_tip(*s->back_button(), [s] { return s->visible(); }, true);
        }

        auto columns = layout::row({heading("Modal bottom sheet", modal_frame_.node(modal_screen)),
                                    heading("Standard bottom sheet", standard_frame_.node(standard_screen)),
                                    heading("Side sheets", side_frame_.node(side_row))});
        columns->spacing(l.column_space).cross_align(layout::CrossAlign::start);
        root = columns;
    }

    bool handle(const input::PointerEvent& e) override {
        source_ = input::InputSource::pointer;
        handle_tips(e);
        if (handle_dialog_tips(e)) {
            return true;
        }
        for (widgets::SideSheet* s : {detached_side_.get(), modal_side_.get()}) {
            if (s->visible()) {
                const widgets::SideSheetResponse r = s->handle(e);
                report_side(*s, r);
                if (r.consumed) {
                    return true;
                }
            }
        }
        if (modal_sheet_->visible()) {
            const widgets::BottomSheetResponse r = modal_sheet_->handle(e);
            if (r.dismissed) {
                ctx.report("Bottom sheet: dismissed");
            }
            if (r.consumed) {
                return true;
            }
        }
        if (standard_sheet_->handle(e).consumed) {
            return true;
        }
        const widgets::SideSheetResponse side = standard_side_->handle(e);
        report_side(*standard_side_, side);
        if (side.consumed) {
            return true;
        }
        return handle_controls(e);
    }

    bool handle(const input::KeyEvent& e) override {
        source_ = input::InputSource::keyboard;
        handle_tips(e);
        handle_dialog_tips(e);
        if (active_side_ && active_side_->visible()) {
            report_side(*active_side_, active_side_->handle(e));
            return true;
        }
        if (modal_sheet_->visible()) {
            if (modal_sheet_->handle(e).dismissed) {
                ctx.report("Bottom sheet: dismissed");
            }
            return true;
        }
        if (e.key != input::Key::tab) {
            if (standard_sheet_->handle(e).consumed) {
                return true;
            }
            const widgets::SideSheetResponse side = standard_side_->handle(e);
            report_side(*standard_side_, side);
            if (side.consumed) {
                return true;
            }
        }
        return handle_controls(e);
    }

    bool handle(const input::WheelEvent& e) override {
        for (widgets::SideSheet* s : {detached_side_.get(), modal_side_.get()}) {
            if (s->visible() && s->handle(e)) {
                return true;
            }
        }
        if (modal_sheet_->visible() && modal_sheet_->handle(e)) {
            return true;
        }
        return standard_sheet_->handle(e) || standard_side_->handle(e);
    }

    void step(double dt) override {
        BasePage::step(dt);
        for (widgets::BottomSheet* s : {modal_sheet_.get(), standard_sheet_.get()}) {
            s->step(dt);
        }
        for (widgets::SideSheet* s : {modal_side_.get(), detached_side_.get(), standard_side_.get()}) {
            s->step(dt);
        }
        if (modal_open_ && !modal_sheet_->visible()) {
            modal_open_ = false;
            focus_.resume();
        }
        if (active_side_ && !active_side_->visible()) {
            active_side_ = nullptr;
            focus_.resume();
        }
        report_settled("Bottom sheet", *modal_sheet_, modal_value_);
        report_settled("Standard sheet", *standard_sheet_, standard_value_);
    }

    bool animating() const override {
        bool any = BasePage::animating() || modal_sheet_->animating() || standard_sheet_->animating();
        for (const widgets::SideSheet* s : {modal_side_.get(), detached_side_.get(), standard_side_.get()}) {
            any = any || s->animating();
        }
        return any;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const DemoFrame* f : {&modal_frame_, &standard_frame_, &side_frame_}) {
            f->paint_background(canvas, scheme);
        }
        BasePage::paint(canvas, scheme);
        standard_frame_.begin_clip(canvas);
        standard_sheet_->paint(canvas, scheme);
        standard_frame_.end_clip(canvas);
        side_frame_.begin_clip(canvas);
        standard_side_->paint(canvas, scheme);
        modal_side_->paint(canvas, scheme);
        detached_side_->paint(canvas, scheme);
        side_frame_.end_clip(canvas);
        modal_frame_.begin_clip(canvas);
        modal_sheet_->paint(canvas, scheme);
        modal_frame_.end_clip(canvas);
        for (const DemoFrame* f : {&modal_frame_, &standard_frame_, &side_frame_}) {
            f->paint_border(canvas, scheme);
        }
    }

    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint_overlays(canvas, scheme);
        paint_dialog_tips(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const widgets::BottomSheet* s : {standard_sheet_.get(), modal_sheet_.get()}) {
            if (auto n = s->accessible()) {
                nodes.push_back(std::move(*n));
            }
        }
        for (const widgets::SideSheet* s : {standard_side_.get(), modal_side_.get(), detached_side_.get()}) {
            if (auto n = s->accessible()) {
                nodes.push_back(std::move(*n));
            }
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
        modal_body_->widgets.set_enabled(enabled);
        standard_body_->widgets.set_enabled(enabled);
        for (auto& b : side_bodies_) {
            b->widgets.set_enabled(enabled);
        }
        for (widgets::SideSheet* s : {modal_side_.get(), detached_side_.get(), standard_side_.get()}) {
            s->set_enabled(enabled);
        }
        standard_sheet_->set_gestures_enabled(enabled);
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        modal_sheet_->cancel_interaction();
        standard_sheet_->cancel_interaction();
        for (widgets::SideSheet* s : {modal_side_.get(), detached_side_.get(), standard_side_.get()}) {
            s->cancel_interaction();
        }
    }

private:
    widgets::ListItemSpec action_item(const char* name, icons::Symbol s) const {
        widgets::ListItemSpec spec{name};
        spec.leading = widgets::ListSlot::of_icon(icon(s));
        return spec;
    }

    widgets::ListItemSpec place_item(const char* name, const char* detail) const {
        widgets::ListItemSpec spec{name, detail};
        spec.leading = widgets::ListSlot::of_icon(icon(res::icons::location_on));
        return spec;
    }

    std::vector<widgets::ListItemSpec> filter_items() const {
        std::vector<widgets::ListItemSpec> items;
        for (const auto& [name, on] : std::array<std::pair<const char*, bool>, 5>{
                 {{"In stock", true}, {"On sale", false}, {"Free shipping", true}, {"New arrivals", false}, {"Top rated", false}}}) {
            widgets::ListItemSpec spec{name};
            spec.trailing = widgets::ListSlot::checkbox();
            spec.action = widgets::ListItemAction::toggle;
            spec.selected = on;
            items.push_back(spec);
        }
        return items;
    }

    widgets::SideSheetOptions side_options(widgets::SideSheetKind kind, bool detached, SheetBody& body) const {
        const bool standard = kind == widgets::SideSheetKind::standard;
        return {.kind = kind,
                .detached = detached,
                .headline = "Filters",
                .close = widgets::SideSheetIcon{icon(res::icons::close), "Close"},
                .back = widgets::SideSheetIcon{icon(res::icons::arrow_back), "Back"},
                .content = body.slot(!standard),
                .actions = {widgets::ButtonOptions{"Save"}, widgets::ButtonOptions{"Cancel", tokens::ButtonStyle::outlined}},
                .accessible_name = standard ? "Standard filters" : detached ? "Detached filters" : "Filters"};
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

    layout::NodePtr screen(const char* title, const char* body, std::vector<widgets::Button*> buttons) {
        const DemoFrameStyle& s = demo_frame_style;
        auto& t = controls.emplace<widgets::Label>(widgets::LabelOptions{title, s.title_type, s.title, false}, ctx.fonts);
        auto& b = controls.emplace<widgets::Label>(widgets::LabelOptions{body, s.body_type, s.body, true}, ctx.fonts);
        std::vector<layout::NodePtr> nodes{widgets::node(t), widgets::node(b)};
        for (widgets::Button* button : buttons) {
            nodes.push_back(widgets::node(*button));
        }
        auto c = layout::column(std::move(nodes));
        c->spacing(s.gap).padding(layout::Insets::all(s.padding));
        return c;
    }

    void open_modal_sheet() {
        if (!modal_open_) {
            focus_.suspend();
            modal_open_ = true;
        }
        modal_sheet_->show(source_);
        ctx.report("Bottom sheet: shown");
    }

    void open_side(widgets::SideSheet& s) {
        if (!active_side_) {
            focus_.suspend();
        }
        active_side_ = &s;
        s.open(source_);
        ctx.report("Side sheet: opened");
    }

    void report_side(widgets::SideSheet&, const widgets::SideSheetResponse& r) {
        if (r.dismissed) {
            ctx.report("Side sheet: dismissed");
        } else if (r.back) {
            ctx.report("Side sheet: back");
        } else if (r.action) {
            ctx.report(*r.action == 0 ? "Side sheet: Save" : "Side sheet: Cancel");
        }
    }

    void report_settled(const char* name, const widgets::BottomSheet& s, widgets::SheetValue& last) {
        if (s.current_value() != last && !s.dragging()) {
            last = s.current_value();
            ctx.report(std::string(name) + ": " + value_name(last));
        }
    }

    DemoFrame modal_frame_;
    DemoFrame standard_frame_;
    DemoFrame side_frame_;
    std::unique_ptr<SheetBody> modal_body_;
    std::unique_ptr<SheetBody> standard_body_;
    std::array<std::unique_ptr<SheetBody>, 3> side_bodies_;
    std::unique_ptr<widgets::BottomSheet> modal_sheet_;
    std::unique_ptr<widgets::BottomSheet> standard_sheet_;
    std::unique_ptr<widgets::SideSheet> modal_side_;
    std::unique_ptr<widgets::SideSheet> detached_side_;
    std::unique_ptr<widgets::SideSheet> standard_side_;
    widgets::SideSheet* active_side_ = nullptr;
    bool modal_open_ = false;
    widgets::SheetValue modal_value_ = widgets::SheetValue::hidden;
    widgets::SheetValue standard_value_ = widgets::SheetValue::partially_expanded;
    input::InputSource source_ = input::InputSource::pointer;
};

}

std::unique_ptr<Page> make_sheets_page(PageContext context) {
    return std::make_unique<SheetsPage>(std::move(context));
}

}
