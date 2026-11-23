#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/search_bar.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct SearchLayout {
    layout::Size phone{412.0, 700.0};
    layout::Size desktop{900.0, 700.0};
    double column_space = 24.0;
    double bar_padding_x = 16.0;
    double bar_padding_y = 8.0;
};

const SearchLayout search_layout;

struct Place {
    const char* name;
    const char* region;
};

constexpr std::array<Place, 14> places{{
    {"Kyoto", "Kyoto Prefecture"},     {"Kobe", "Hyogo Prefecture"},       {"Kanazawa", "Ishikawa Prefecture"},
    {"Kagoshima", "Kagoshima Prefecture"}, {"Kamakura", "Kanagawa Prefecture"}, {"Kochi", "Kochi Prefecture"},
    {"Kumamoto", "Kumamoto Prefecture"}, {"Kurashiki", "Okayama Prefecture"},  {"Nara", "Nara Prefecture"},
    {"Nagasaki", "Nagasaki Prefecture"}, {"Osaka", "Osaka Prefecture"},       {"Sapporo", "Hokkaido"},
    {"Sendai", "Miyagi Prefecture"},     {"Takayama", "Gifu Prefecture"},
}};

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

class SearchPage final : public BasePage {
public:
    explicit SearchPage(PageContext context)
        : BasePage(std::move(context)), compact_frame_(search_layout.phone), wide_frame_(search_layout.desktop) {
        compact_ = make_bar(widgets::SearchViewMode::automatic);
        wide_ = make_bar(widgets::SearchViewMode::docked);
        compact_frame_.on_arrange([this](const render::Rect& r) { compact_->set_area(r); });
        wide_frame_.on_arrange([this](const render::Rect& r) { wide_->set_area(r); });
        auto compact_screen = screen(*compact_, "Trips", "Search for a city. On a compact window the search view fills the screen.");
        auto wide_screen = screen(*wide_, "Trips", "On a wide window the results drop down below the bar, up to two thirds of the window.");
        for (widgets::SearchBar* s : {compact_.get(), wide_.get()}) {
            s->add_focus_stops(focus_);
            gated_tip(s->back_button(), [s] { return s->is_open(); }, true);
            gated_tip(*s->trailing_button(), [] { return true; }, true);
        }
        root = columns({{heading("Search bar, full-screen view", compact_frame_.node(compact_screen), layout::CrossAlign::stretch)},
                        {heading("Search bar, docked view", wide_frame_.node(wide_screen), layout::CrossAlign::stretch)}},
                       search_layout.column_space, search_layout.column_space);
    }

    bool holds_pointer() const override {
        return compact_->holds_pointer() || wide_->holds_pointer() || BasePage::holds_pointer();
    }
    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        if (handle_dialog_tips(e)) {
            return true;
        }
        bool used = false;
        for (widgets::SearchBar* s : bars()) {
            if (s->is_open()) {
                const widgets::SearchBarResponse r = s->handle(e);
                respond(*s, r);
                used = used || r.consumed;
            }
        }
        if (used) {
            return true;
        }
        for (widgets::SearchBar* s : bars()) {
            if (!s->is_open()) {
                const widgets::SearchBarResponse r = s->handle(e);
                respond(*s, r);
                if (r.consumed) {
                    return true;
                }
            }
        }
        return handle_controls(e);
    }

    bool handle(const input::KeyEvent& e) override {
        handle_tips(e);
        handle_dialog_tips(e);
        for (widgets::SearchBar* s : bars()) {
            if (s->is_open()) {
                const widgets::SearchBarResponse r = s->handle(e);
                respond(*s, r);
                if (r.consumed) {
                    return true;
                }
            }
        }
        if (e.key != input::Key::tab) {
            for (widgets::SearchBar* s : bars()) {
                if (s->focused()) {
                    const widgets::SearchBarResponse r = s->handle(e);
                    respond(*s, r);
                    if (r.consumed) {
                        return true;
                    }
                }
            }
        }
        return handle_controls(e);
    }

    void handle(const input::TextEvent& e) override {
        for (widgets::SearchBar* s : bars()) {
            if (s->focused()) {
                respond(*s, s->handle(e));
                return;
            }
        }
        controls.handle(e);
    }

    bool handle(const input::WheelEvent& e) override {
        for (widgets::SearchBar* s : bars()) {
            if (s->handle(e)) {
                return true;
            }
        }
        return false;
    }

    std::optional<render::Rect> caret_area() const override {
        for (const widgets::SearchBar* s : bars()) {
            if (const auto c = s->caret_area()) {
                return c;
            }
        }
        return std::nullopt;
    }

    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        for (const widgets::SearchBar* s : bars()) {
            if (const auto w = s->next_wake_seconds()) {
                wake = wake ? std::min(*wake, *w) : *w;
            }
        }
        return wake;
    }

    void step(double dt) override {
        BasePage::step(dt);
        for (widgets::SearchBar* s : bars()) {
            s->step(dt);
        }
    }

    bool animating() const override {
        bool any = BasePage::animating();
        for (const widgets::SearchBar* s : bars()) {
            any = any || s->animating();
        }
        return any;
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const DemoFrame* f : {&compact_frame_, &wide_frame_}) {
            f->paint_background(canvas, scheme);
        }
        BasePage::paint(canvas, scheme);
        compact_->paint(canvas, scheme);
        wide_->paint(canvas, scheme);
        compact_frame_.begin_clip(canvas);
        compact_->paint_expanded(canvas, scheme);
        compact_frame_.end_clip(canvas);
        wide_frame_.begin_clip(canvas);
        wide_->paint_expanded(canvas, scheme);
        wide_frame_.end_clip(canvas);
        for (const DemoFrame* f : {&compact_frame_, &wide_frame_}) {
            f->paint_border(canvas, scheme);
        }
    }

    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint_overlays(canvas, scheme);
        paint_dialog_tips(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const widgets::SearchBar* s : bars()) {
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
        for (widgets::SearchBar* s : bars()) {
            s->set_enabled(enabled);
        }
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        for (widgets::SearchBar* s : bars()) {
            s->cancel_interaction();
            s->collapse();
        }
    }

private:
    std::array<widgets::SearchBar*, 2> bars() const { return {compact_.get(), wide_.get()}; }

    std::unique_ptr<widgets::SearchBar> make_bar(widgets::SearchViewMode mode) const {
        return std::make_unique<widgets::SearchBar>(widgets::SearchBarOptions{.placeholder = "Search cities",
                                                                              .search_icon = icon(res::icons::search),
                                                                              .back = {icon(res::icons::arrow_back), "Back"},
                                                                              .trailing = widgets::SearchBarIcon{icon(res::icons::mic), "Mic"},
                                                                              .results = results_for(""),
                                                                              .mode = mode},
                                                    ctx.fonts);
    }

    std::vector<widgets::ListItemSpec> results_for(const std::string& query) const {
        std::vector<widgets::ListItemSpec> out;
        const std::string q = lower(query);
        for (const Place& p : places) {
            if (lower(p.name).find(q) == std::string::npos) {
                continue;
            }
            widgets::ListItemSpec spec{p.name, p.region};
            spec.leading = widgets::ListSlot::of_icon(icon(res::icons::location_on));
            spec.transparent = true;
            out.push_back(spec);
        }
        return out;
    }

    layout::NodePtr screen(widgets::SearchBar& bar, const char* title, const char* text) {
        const DemoFrameStyle& s = demo_frame_style;
        auto& t = controls.emplace<widgets::Label>(widgets::LabelOptions{title, s.title_type, s.title, false}, ctx.fonts);
        auto& b = controls.emplace<widgets::Label>(widgets::LabelOptions{text, s.body_type, s.body, true}, ctx.fonts);
        auto bar_row = widgets::measured_node(bar);
        bar_row->self_align = layout::CrossAlign::stretch;
        auto body = layout::column({widgets::node(t), widgets::node(b)});
        body->spacing(s.gap).padding(layout::Insets::all(s.padding));
        auto top = layout::column({bar_row});
        top->padding({search_layout.bar_padding_x, search_layout.bar_padding_y, search_layout.bar_padding_x, search_layout.bar_padding_y});
        top->fill_cross(true);
        auto c = layout::column({top, body});
        c->fill_cross(true);
        return c;
    }

    void respond(widgets::SearchBar& s, const widgets::SearchBarResponse& r) {
        if (r.query_changed) {
            s.set_results(results_for(s.query()));
        }
        if (r.chosen) {
            const std::string name = s.results().item(*r.chosen).headline;
            s.set_query(name);
            s.set_results(results_for(name));
            s.collapse();
            ctx.report("Search: " + name);
        } else if (r.searched) {
            s.collapse();
            ctx.report("Search: " + *r.searched);
        } else if (r.trailing) {
            ctx.report("Search: Mic");
        } else if (r.expanded) {
            ctx.report("Search: expanded");
        } else if (r.collapsed) {
            ctx.report("Search: collapsed");
        }
    }

    DemoFrame compact_frame_;
    DemoFrame wide_frame_;
    std::unique_ptr<widgets::SearchBar> compact_;
    std::unique_ptr<widgets::SearchBar> wide_;
};

}

std::unique_ptr<Page> make_search_page(PageContext context) {
    return std::make_unique<SearchPage>(std::move(context));
}

}
