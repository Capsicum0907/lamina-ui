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
#include "components/reflow.hpp"
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
    double line_gap = 8.0;
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
            const double w = std::min(width, c.max_width);
            const layout::Size s = node->measure({0.0, w, 0.0, c.max_height});
            return layout::Size{w, s.height};
        },
        [node](const render::Rect& r) { node->arrange(r); });
}

inline layout::NodePtr sized(layout::NodePtr node, double width, double height) {
    return std::make_shared<layout::Leaf>(
        [node, width, height](const layout::Constraints& c) {
            const double w = std::min(width, c.max_width);
            node->measure(layout::Constraints::tight({w, height}));
            return layout::Size{w, height};
        },
        [node](const render::Rect& r) { node->arrange(r); });
}

class Deferred final : public layout::Node {
public:
    explicit Deferred(const layout::NodePtr& target) : target_(target) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return target_->measure(c); }
    void on_arrange(const render::Rect& r) override { target_->arrange(r); }

private:
    const layout::NodePtr& target_;
};

class BasePage : public Page {
public:
    explicit BasePage(PageContext context)
        : ctx(std::move(context)), reflow(*ctx.size_class), body_(reflow.root(std::make_shared<Deferred>(root))) {
        ctx.add_header(focus_);
    }

    layout::NodePtr body() const override { return body_; }
    void snap_layout() override { reflow.snap(); }
    std::vector<render::Rect> layout_blocks() const override { return reflow.blocks(); }
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
        reflow.step(dt);
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
        return reflow.animating() || controls.animating() || std::any_of(tooltips.begin(), tooltips.end(), [](const auto& t) { return t->animating(); }) ||
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
        return std::make_shared<Captioned>(reflow, examples::caption_cell(*captions.back(), base_page_layout.label_width, style.line_height, style.line_height),
                                           std::move(content), base_page_layout.heading_gap);
    }

    layout::NodePtr columns(const std::vector<std::vector<layout::NodePtr>>& sections, double column_gap, double row_gap) {
        return std::make_shared<Columns>(reflow, sections, column_gap, row_gap);
    }

    layout::NodePtr heading(const char* text, layout::NodePtr content, layout::CrossAlign align = layout::CrossAlign::start) {
        captions.push_back(
            std::make_unique<widgets::Label>(widgets::LabelOptions{text, base_page_layout.heading_type, base_page_layout.heading, false}, ctx.fonts));
        auto c = layout::column({widgets::node(*captions.back()), content});
        c->spacing(base_page_layout.heading_gap).cross_align(align);
        return c;
    }

    std::shared_ptr<layout::Flow> items(std::vector<layout::NodePtr> nodes) {
        auto f = reflow.flow(std::move(nodes));
        f->spacing(base_page_layout.column_gap).line_spacing(base_page_layout.line_gap).cross_align(layout::CrossAlign::center);
        return f;
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

    void handle_tips(const input::PointerEvent& e) {
        for (auto& t : tooltips) {
            t->handle(e);
        }
    }
    void handle_tips(const input::KeyEvent& e) {
        for (auto& t : tooltips) {
            t->handle(e);
        }
    }
    bool handle_controls(const input::PointerEvent& e) { return controls.handle(e); }
    bool handle_controls(const input::KeyEvent& e) { return focus_.handle(e) || controls.handle(e); }

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
    Reflow reflow;
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

private:
    layout::NodePtr body_;
};

}
