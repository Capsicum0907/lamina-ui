#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/loading_indicator.hpp"
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
};

const ProgressLayout progress_layout;

std::string percent(double v) {
    return std::to_string(static_cast<long long>(std::lround(v * progress_layout.percent))) + " %";
}

class ProgressPage final : public BasePage {
public:
    explicit ProgressPage(PageContext context) : BasePage(std::move(context)) {
        const ProgressLayout& l = progress_layout;
        root = columns({{loading(), controls_section()}, {wavy()}}, l.column_space, l.section_gap);
        set_all(l.start_progress, false);
    }

    void step(double dt) override {
        BasePage::step(dt);
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

    bool animating() const override { return BasePage::animating() || running_; }

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
                    ctx.report("Progress: " + percent(s.fraction()));
                }
            },
            widgets::SliderOptions{.value = l.start_progress, .accessible_name = "Progress", .value_label = percent}, ctx.fonts);
        slider_ = &slider;
        focus_.add(slider);
        enabler("Progress", slider);
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
        rows.push_back(captioned("Progress", width_limited(widgets::node(slider), l.slider_width)));
        rows.push_back(captioned("Run", widgets::centered_node(run)));
        rows.push_back(captioned("Indeterminate", widgets::centered_node(indeterminate)));
        auto column = layout::column(std::move(rows));
        column->spacing(progress_layout.row_gap);
        return heading("Drive the determinate indicators", column);
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
};

}

std::unique_ptr<Page> make_progress_page(PageContext context) {
    return std::make_unique<ProgressPage>(std::move(context));
}

}
