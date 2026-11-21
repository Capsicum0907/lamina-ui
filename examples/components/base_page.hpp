#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/caption_cell.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/tooltip.hpp"
#include "lamina/widgets/widget_set.hpp"

namespace lamina::examples::components {

struct BasePageLayout {
    double label_width = 120.0;
    double column_gap = 16.0;
    tokens::TypeRole caption_type = tokens::TypeRole::label_large;
    color::Role caption = color::Role::on_surface_variant;
    tokens::TypeRole heading_type = tokens::TypeRole::title_small;
    color::Role heading = color::Role::on_surface;
    double heading_gap = 8.0;
};

inline const BasePageLayout base_page_layout;

struct TipAnchor {
    std::function<render::Rect()> rect;
    std::function<bool()> has_focus;
    render::Rect bounds() const { return rect(); }
    bool focused() const { return has_focus(); }
    void set_focused(bool) {}
};

struct LiveTip {
    widgets::Tooltip* tip;
    std::function<std::string()> text;
    std::string shown;
};

inline layout::NodePtr width_limited(layout::NodePtr node, double width) {
    return std::make_shared<layout::Leaf>(
        [node, width](const layout::Constraints& c) {
            const layout::Size s = node->measure({0.0, width, 0.0, c.max_height});
            return layout::Size{width, s.height};
        },
        [node](const render::Rect& r) { node->arrange(r); });
}

inline layout::NodePtr sized(layout::NodePtr node, double width, double height) {
    return std::make_shared<layout::Leaf>(
        [node, width, height](const layout::Constraints&) {
            node->measure(layout::Constraints::tight({width, height}));
            return layout::Size{width, height};
        },
        [node](const render::Rect& r) { node->arrange(r); });
}

class BasePage : public Page {
public:
    explicit BasePage(PageContext context) : ctx(std::move(context)) { ctx.add_header(focus_); }

    layout::NodePtr body() const override { return root; }
    widgets::FocusManager& focus() override { return focus_; }

    bool handle(const input::PointerEvent& e) override {
        bool consumed = false;
        for (auto& t : tooltips) {
            consumed = t->handle(e).consumed || consumed;
        }
        if (consumed) {
            return true;
        }
        return controls.handle(e);
    }
    bool handle(const input::KeyEvent& e) override {
        bool consumed = false;
        for (auto& t : tooltips) {
            consumed = t->handle(e).consumed || consumed;
        }
        if (consumed || focus_.handle(e)) {
            return true;
        }
        return controls.handle(e);
    }
    bool handle(const input::WheelEvent&) override { return false; }
    void step(double dt) override {
        controls.step(dt);
        for (auto& t : tooltips) {
            t->step(dt);
        }
        for (auto& t : dialog_tips) {
            t->step(dt);
        }
        for (LiveTip& l : live_tips) {
            std::string text = l.text();
            if (text != l.shown) {
                l.tip->set_text(text);
                l.shown = std::move(text);
            }
        }
    }
    bool animating() const override {
        return controls.animating() || std::any_of(tooltips.begin(), tooltips.end(), [](const auto& t) { return t->animating(); }) ||
               std::any_of(dialog_tips.begin(), dialog_tips.end(), [](const auto& t) { return t->animating(); });
    }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const auto& c : captions) {
            c->paint(canvas, scheme);
        }
        controls.paint(canvas, scheme);
    }
    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        for (const auto& t : tooltips) {
            t->paint(canvas, scheme);
        }
    }
    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = controls.accessible_nodes();
        for (const auto& t : tooltips) {
            if (auto n = t->accessible()) {
                nodes.push_back(std::move(*n));
            }
        }
        return nodes;
    }
    void set_enabled(bool enabled) override { controls.set_enabled(enabled); }
    bool set_enabled(std::string_view name, bool enabled) override {
        const auto it = enablers.find(name);
        if (it == enablers.end()) {
            return false;
        }
        it->second(enabled);
        return true;
    }
    void cancel_interaction() override {
        controls.cancel_interaction();
        for (auto& t : tooltips) {
            t->dismiss();
        }
        for (auto& t : dialog_tips) {
            t->dismiss();
        }
    }
    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = focus_.next_wake_seconds();
        for (const auto& t : dialog_tips) {
            if (const auto w = t->next_wake_seconds()) {
                wake = wake ? std::min(*wake, *w) : *w;
            }
        }
        for (const auto& t : tooltips) {
            if (const auto w = t->next_wake_seconds()) {
                wake = wake ? std::min(*wake, *w) : *w;
            }
        }
        return wake;
    }
    void set_window(layout::Size window) override {
        for (auto& t : tooltips) {
            t->set_window(window);
        }
        for (auto& t : dialog_tips) {
            t->set_window(window);
        }
    }

protected:
    icons::Icon icon(icons::Symbol s) const { return {ctx.icon_font, s}; }

    layout::NodePtr captioned(const char* caption, layout::NodePtr content) {
        const auto style = tokens::text_style(tokens::type_style(base_page_layout.caption_type));
        captions.push_back(std::make_unique<widgets::Label>(
            widgets::LabelOptions{caption, base_page_layout.caption_type, base_page_layout.caption, false}, ctx.fonts));
        auto r = layout::row({examples::caption_cell(*captions.back(), base_page_layout.label_width, style.line_height, style.line_height), content});
        r->cross_align(layout::CrossAlign::center);
        return r;
    }

    layout::NodePtr heading(const char* text, layout::NodePtr content) {
        captions.push_back(
            std::make_unique<widgets::Label>(widgets::LabelOptions{text, base_page_layout.heading_type, base_page_layout.heading, false}, ctx.fonts));
        auto c = layout::column({widgets::node(*captions.back()), content});
        c->spacing(base_page_layout.heading_gap);
        return c;
    }

    layout::NodePtr items(std::vector<layout::NodePtr> nodes) {
        auto r = layout::row(std::move(nodes));
        r->spacing(base_page_layout.column_gap).cross_align(layout::CrossAlign::center);
        return r;
    }

    template <typename W>
    widgets::Tooltip& gated_tip(const W& w, std::function<bool()> shown, bool in_dialog = false) {
        tip_anchors.push_back(std::make_unique<TipAnchor>(
            TipAnchor{[&w, shown] { return shown() ? w.bounds() : render::Rect{0.0, 0.0, 0.0, 0.0}; },
                      [&w, shown] { return shown() && w.focused(); }}));
        return live_tip([&w] { return w.accessible().name; }, *tip_anchors.back(), in_dialog);
    }

    widgets::Tooltip& live_tip(std::function<std::string()> text, const TipAnchor& anchor, bool in_dialog) {
        auto& list = in_dialog ? dialog_tips : tooltips;
        std::string initial = text();
        list.push_back(std::make_unique<widgets::Tooltip>(widgets::TooltipOptions{initial}, ctx.fonts));
        list.back()->anchor_to(anchor);
        live_tips.push_back({list.back().get(), std::move(text), std::move(initial)});
        return *list.back();
    }

    bool handle_dialog_tips(const input::PointerEvent& e) {
        bool consumed = false;
        for (auto& t : dialog_tips) {
            consumed = t->handle(e).consumed || consumed;
        }
        return consumed;
    }
    bool handle_dialog_tips(const input::KeyEvent& e) {
        bool consumed = false;
        for (auto& t : dialog_tips) {
            consumed = t->handle(e).consumed || consumed;
        }
        return consumed;
    }
    void paint_dialog_tips(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        for (const auto& t : dialog_tips) {
            t->paint(canvas, scheme);
        }
    }

    template <typename W>
    void name_tip(const W& w) {
        tooltips.push_back(std::make_unique<widgets::Tooltip>(widgets::name_tooltip(w, ctx.fonts)));
    }

    template <typename W>
    void enabler(const std::string& name, W& w) {
        enablers[name] = [&w](bool e) { w.set_enabled(e); };
    }

    PageContext ctx;
    widgets::WidgetSet controls;
    widgets::FocusManager focus_;
    std::vector<std::unique_ptr<widgets::Label>> captions;
    std::vector<std::unique_ptr<widgets::Tooltip>> tooltips;
    std::vector<std::unique_ptr<widgets::Tooltip>> dialog_tips;
    std::vector<std::unique_ptr<TipAnchor>> tip_anchors;
    std::vector<LiveTip> live_tips;
    std::vector<std::unique_ptr<widgets::WidgetSet>> contents;
    std::map<std::string, std::function<void(bool)>, std::less<>> enablers;
    layout::NodePtr root;
};

}
