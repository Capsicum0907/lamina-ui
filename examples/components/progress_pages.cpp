#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/demo_frame.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/loading_indicator.hpp"
#include "lamina/widgets/pull_to_refresh.hpp"
#include "lamina/widgets/slider.hpp"
#include "lamina/widgets/switch.hpp"
#include "lamina/widgets/wavy_progress.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

namespace res = examples::res;
using widgets::CircularWavyProgressIndicator;
using widgets::LinearWavyProgressIndicator;
using widgets::LoadingIndicator;

struct ProgressLayout {
    double column_space = 72.0;
    double row_gap = 28.0;
    double section_gap = 36.0;
    double linear_width = 420.0;
    double slider_width = 300.0;
    double start_progress = 0.6;
    double run_step = 0.08;
    double run_interval = 0.4;
    double percent = 100.0;
    double large = 144.0;
    double handoff_inset = 64.0;
    layout::Size refresh_frame{320.0, 400.0};
    std::size_t refresh_rows = 16;
    double refresh_seconds = 1.5;
};

const ProgressLayout progress_layout;

constexpr const char* value_name = "Indicator value";

std::string percent(double v) {
    return std::to_string(static_cast<long long>(std::lround(v * progress_layout.percent))) + " %";
}

class ProgressPage final : public BasePage {
public:
    explicit ProgressPage(PageContext context) : BasePage(std::move(context)) {
        const ProgressLayout& l = progress_layout;
        root = columns({{controls_section(), loading()}, {wavy(), refresh()}}, l.column_space, l.section_gap);
        set_all(l.start_progress, false);
    }

    bool handle(const input::PointerEvent& e) override {
        handle_tips(e);
        const bool at_top = list_->scroll_area().offset() <= tokens::pull_to_refresh_tokens.top_tolerance;
        const widgets::PullToRefreshResponse r = pull_.handle(e, at_top);
        if (r.refresh) {
            refresh_clock_ = 0.0;
            ctx.report("Pull to refresh: refreshing");
        }
        if (r.claimed) {
            list_->cancel_interaction();
        }
        if (r.consumed) {
            return true;
        }
        const bool used = list_->handle(e).consumed;
        return handle_controls(e) || used;
    }
    bool handle(const input::WheelEvent& e) override { return list_->handle(e); }
    bool holds_pointer() const override { return pull_.holds_pointer() || list_->holds_pointer() || BasePage::holds_pointer(); }

    void step(double dt) override {
        BasePage::step(dt);
        list_->step(dt);
        if (pull_.refreshing()) {
            refresh_clock_ += dt;
            if (refresh_clock_ >= progress_layout.refresh_seconds) {
                refreshed_ += 1;
                std::vector<widgets::ListItemSpec> rows = refresh_rows();
                list_->set_items(std::move(rows));
                pull_.set_refreshing(false);
                ctx.report("Pull to refresh: done");
            }
        }
        pull_.step(dt);
        if (running_) {
            run_clock_ += dt;
            if (run_clock_ >= progress_layout.run_interval) {
                run_clock_ -= progress_layout.run_interval;
                const double next = std::min(1.0, shown_ + progress_layout.run_step);
                set_all(next, true);
                if (next >= 1.0) {
                    running_ = false;
                    ctx.report("Progress: done");
                }
            }
        }
    }

    bool animating() const override { return BasePage::animating() || running_ || list_->animating() || pull_.animating(); }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        frame_.paint_background(canvas, scheme);
        BasePage::paint(canvas, scheme);
        frame_.begin_clip(canvas);
        list_->paint(canvas, scheme);
        pull_.paint(canvas, scheme);
        frame_.end_clip(canvas);
        frame_.paint_border(canvas, scheme);
    }

    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        nodes.push_back(list_->accessible());
        if (auto n = pull_.accessible()) {
            nodes.push_back(std::move(*n));
        }
        return nodes;
    }

    void set_enabled(bool enabled) override {
        BasePage::set_enabled(enabled);
        list_->set_enabled(enabled);
        pull_.set_enabled(enabled);
    }

    void cancel_interaction() override {
        BasePage::cancel_interaction();
        list_->cancel_interaction();
        pull_.cancel_interaction();
    }

    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        const double w = list_->next_wake_seconds();
        if (w >= 0.0) {
            wake = wake ? std::min(*wake, w) : w;
        }
        return wake;
    }

private:
    layout::NodePtr loading() {
        std::vector<layout::NodePtr> rows;
        auto make = [this](widgets::LoadingIndicatorOptions o) -> LoadingIndicator& {
            auto& li = controls.emplace<LoadingIndicator>(std::move(o));
            return li;
        };
        auto& plain = make({.accessible_name = "Loading"});
        auto& contained = make({.contained = true, .accessible_name = "Loading, contained"});
        rows.push_back(captioned("Indeterminate", items({widgets::centered_node(plain), widgets::centered_node(contained)})));
        auto& det = make({.progress = 0.0, .accessible_name = "Loaded"});
        auto& det_contained = make({.progress = 0.0, .contained = true, .accessible_name = "Loaded, contained"});
        loaders_ = {&det, &det_contained};
        rows.push_back(captioned("Determinate", items({widgets::centered_node(det), widgets::centered_node(det_contained)})));
        auto& big = make({.accessible_name = "Loading, large", .size = progress_layout.large});
        auto& big_contained = make({.contained = true, .accessible_name = "Loading, large and contained", .size = progress_layout.large});
        auto& big_wavy = controls.emplace<CircularWavyProgressIndicator>(widgets::WavyProgressOptions{.accessible_name = "Syncing, large"});
        big_wavy.set_size(progress_layout.large);
        rows.push_back(captioned("Large", items({widgets::centered_node(big), widgets::centered_node(big_contained), widgets::centered_node(big_wavy)})));
        auto column = layout::column(std::move(rows));
        column->spacing(progress_layout.row_gap);
        return heading("Loading indicators", column);
    }

    layout::NodePtr controls_section() {
        const ProgressLayout& l = progress_layout;
        auto& slider = controls.emplace_with<widgets::Slider>(
            [this](widgets::Slider& s, bool changed) {
                if (changed) {
                    running_ = false;
                    set_all(s.fraction(), false);
                    ctx.report(std::string(value_name) + ": " + percent(s.fraction()));
                }
            },
            widgets::SliderOptions{.value = l.start_progress, .accessible_name = value_name, .value_label = percent}, ctx.fonts);
        slider_ = &slider;
        focus_.add(slider);
        enabler(value_name, slider);
        auto& run = controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    running_ = true;
                    run_clock_ = 0.0;
                    set_all(0.0, false);
                    ctx.report("Progress: running");
                }
            },
            widgets::ButtonOptions{"Run", tokens::ButtonStyle::filled, tokens::ButtonSize::small, tokens::ButtonShape::round, icon(res::icons::play_arrow)},
            ctx.fonts);
        focus_.add(run);
        enabler("Run", run);
        auto& indeterminate = controls.emplace_with<widgets::Switch>(
            [this](widgets::Switch& s, bool clicked) {
                if (clicked) {
                    for (auto* p : handoff_linear_) {
                        p->set_progress(s.checked() ? std::nullopt : std::optional(shown_));
                    }
                    for (auto* p : handoff_circular_) {
                        p->set_progress(s.checked() ? std::nullopt : std::optional(shown_));
                    }
                    ctx.report(s.checked() ? "Handoff: indeterminate" : "Handoff: determinate");
                }
            },
            widgets::SwitchOptions{.checked = true, .accessible_name = "Indeterminate"});
        focus_.add(indeterminate);
        enabler("Indeterminate", indeterminate);
        std::vector<layout::NodePtr> rows;
        rows.push_back(captioned("Value", width_limited(widgets::node(slider), l.slider_width)));
        rows.push_back(captioned("Run", widgets::centered_node(run)));
        rows.push_back(captioned("Indeterminate", widgets::centered_node(indeterminate)));
        auto column = layout::column(std::move(rows));
        column->spacing(progress_layout.row_gap);
        return heading("Set the value the indicators show", column);
    }

    layout::NodePtr wavy() {
        const ProgressLayout& l = progress_layout;
        std::vector<layout::NodePtr> rows;
        auto& linear = controls.emplace<LinearWavyProgressIndicator>(widgets::WavyProgressOptions{.progress = 0.0, .accessible_name = "Download"});
        linears_.push_back(&linear);
        rows.push_back(captioned("Linear", width_limited(widgets::node(linear), l.linear_width)));
        auto& linear_ind = controls.emplace<LinearWavyProgressIndicator>(widgets::WavyProgressOptions{.accessible_name = "Waiting"});
        rows.push_back(captioned("Linear, loading", width_limited(widgets::node(linear_ind), l.linear_width)));
        auto& circular = controls.emplace<CircularWavyProgressIndicator>(widgets::WavyProgressOptions{.progress = 0.0, .accessible_name = "Upload"});
        circulars_.push_back(&circular);
        auto& circular_ind = controls.emplace<CircularWavyProgressIndicator>(widgets::WavyProgressOptions{.accessible_name = "Syncing"});
        rows.push_back(captioned("Circular", items({widgets::centered_node(circular), widgets::centered_node(circular_ind)})));
        auto& hand_linear = controls.emplace<LinearWavyProgressIndicator>(widgets::WavyProgressOptions{.accessible_name = "Handoff linear"});
        auto& hand_circular = controls.emplace<CircularWavyProgressIndicator>(widgets::WavyProgressOptions{.accessible_name = "Handoff circular"});
        handoff_linear_.push_back(&hand_linear);
        handoff_circular_.push_back(&hand_circular);
        rows.push_back(captioned("Handoff", items({width_limited(widgets::node(hand_linear), l.linear_width - l.handoff_inset), widgets::centered_node(hand_circular)})));
        auto column = layout::column(std::move(rows));
        column->spacing(progress_layout.row_gap);
        return heading("Wavy progress indicators", column);
    }

    std::vector<widgets::ListItemSpec> refresh_rows() const {
        std::vector<widgets::ListItemSpec> rows;
        for (std::size_t i = 0; i < refreshed_; ++i) {
            widgets::ListItemSpec s{"New message " + std::to_string(refreshed_ - i), "Arrived with refresh " + std::to_string(refreshed_ - i)};
            s.leading = widgets::ListSlot::of_avatar("N");
            rows.push_back(std::move(s));
        }
        for (std::size_t i = 0; i < progress_layout.refresh_rows; ++i) {
            widgets::ListItemSpec s{"Message " + std::to_string(i + 1), i % 2 == 0 ? "Supporting text" : ""};
            s.leading = widgets::ListSlot::of_avatar("M");
            rows.push_back(std::move(s));
        }
        return rows;
    }

    layout::NodePtr refresh() {
        list_ = std::make_unique<widgets::ListView>(widgets::ListViewOptions{refresh_rows(), false, "Refresh list"}, ctx.fonts);
        frame_.on_arrange([this](const render::Rect& r) {
            list_->set_viewport(r);
            pull_.set_area(r);
        });
        auto& arrow = controls.emplace_with<widgets::Switch>(
            [this](widgets::Switch& s, bool clicked) {
                if (clicked) {
                    pull_.set_style(s.checked() ? widgets::PullIndicatorStyle::arrow : widgets::PullIndicatorStyle::expressive);
                    ctx.report(s.checked() ? "Pull to refresh: arrow" : "Pull to refresh: expressive");
                }
            },
            widgets::SwitchOptions{.checked = false, .accessible_name = "Arrow indicator"});
        focus_.add(arrow);
        enabler("Arrow indicator", arrow);
        auto column = layout::column({captioned("Arrow indicator", widgets::centered_node(arrow)), frame_.node(nullptr)});
        column->spacing(progress_layout.row_gap);
        return heading("Pull to refresh", column, layout::CrossAlign::start);
    }

    void set_all(double v, bool ease) {
        shown_ = v;
        for (auto* li : loaders_) {
            li->set_progress(v);
        }
        for (auto* p : linears_) {
            ease ? p->ease_progress(v) : p->set_progress(v);
        }
        for (auto* p : circulars_) {
            ease ? p->ease_progress(v) : p->set_progress(v);
        }
        for (auto* p : handoff_linear_) {
            if (p->determinate()) {
                p->set_progress(v);
            }
        }
        for (auto* p : handoff_circular_) {
            if (p->determinate()) {
                p->set_progress(v);
            }
        }
        if (slider_ && std::abs(slider_->fraction() - v) > 1e-9 && ease) {
            slider_->set_value(v);
        }
    }

    std::vector<LoadingIndicator*> loaders_;
    std::vector<LinearWavyProgressIndicator*> linears_;
    std::vector<CircularWavyProgressIndicator*> circulars_;
    std::vector<LinearWavyProgressIndicator*> handoff_linear_;
    std::vector<CircularWavyProgressIndicator*> handoff_circular_;
    widgets::Slider* slider_ = nullptr;
    double shown_ = 0.0;
    bool running_ = false;
    double run_clock_ = 0.0;
    DemoFrame frame_{progress_layout.refresh_frame};
    std::unique_ptr<widgets::ListView> list_;
    widgets::PullToRefresh pull_{widgets::PullToRefreshOptions{.accessible_name = "Refreshing"}};
    double refresh_clock_ = 0.0;
    std::size_t refreshed_ = 0;
};

}

std::unique_ptr<Page> make_progress_page(PageContext context) {
    return std::make_unique<ProgressPage>(std::move(context));
}

}
