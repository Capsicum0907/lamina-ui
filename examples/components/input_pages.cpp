#include <cmath>
#include <string>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/widgets/range_slider.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/slider.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

namespace res = examples::res;
using widgets::Segment;
using widgets::SegmentSelection;

struct InputsLayout {
    double row_gap = 24.0;
    double column_space = 64.0;
    double slider_width = 360.0;
    double vertical_length = 220.0;
    double vertical_gap = 40.0;
    double price_min = 0.0;
    double price_max = 500.0;
    double price_start = 120.0;
    double price_end = 360.0;
    int tick_steps = 9;
    double balance_min = -50.0;
    double balance_max = 50.0;
    double custom_corner = 4.0;
    double percent = 100.0;
};

const InputsLayout inputs_layout;

std::string price_label(double v) {
    return "$" + std::to_string(static_cast<long long>(std::lround(v)));
}

std::string whole_label(double v) {
    return std::to_string(static_cast<long long>(std::lround(v)));
}

std::string percent_label(double v) {
    return std::to_string(static_cast<long long>(std::lround(v * inputs_layout.percent))) + " %";
}

class InputsPage final : public BasePage {
public:
    explicit InputsPage(PageContext context) : BasePage(std::move(context)) {
        root = columns({{segments()}, {ranges(), sliders()}}, inputs_layout.column_space, inputs_layout.row_gap);
    }

private:
    widgets::SegmentedButtonRow& segmented(std::vector<layout::NodePtr>& rows, const char* caption, widgets::SegmentedButtonRowOptions options) {
        auto& r = controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow& w, const widgets::SegmentedButtonResponse& response) {
                if (!response.clicked) {
                    return;
                }
                const std::size_t i = *response.clicked;
                ctx.report("Segment: " + w.segment_name(i) + (w.selection() == SegmentSelection::multiple ? (w.checked(i) ? " on" : " off") : ""));
            },
            std::move(options), ctx.fonts, ctx.icon_font);
        focus_.add(r);
        enabler(caption, r);
        rows.push_back(captioned(caption, widgets::node(r)));
        last_segmented_ = &r;
        return r;
    }

    layout::NodePtr segments() {
        std::vector<layout::NodePtr> rows;
        segmented(rows, "Single", {{{"Day"}, {"Week", std::nullopt, true}, {"Month"}, {"Year"}}, SegmentSelection::single, "View"});
        segmented(rows, "With icons",
                  {{{"Walk", icon(res::icons::directions_walk), true}, {"Transit", icon(res::icons::directions_transit)},
                    {"Drive", icon(res::icons::directions_car)}},
                   SegmentSelection::single,
                   "Travel mode"});
        segmented(rows, "Multiple",
                  {{{"XS"}, {"S", std::nullopt, true}, {"M", std::nullopt, true}, {"L"}, {"XL"}}, SegmentSelection::multiple, "Sizes"});
        segmented(rows, "Icons only",
                  {{{"", icon(res::icons::format_bold), true, true, "Bold"},
                    {"", icon(res::icons::format_italic), false, true, "Italic"},
                    {"", icon(res::icons::format_underlined), false, true, "Underline"}},
                   SegmentSelection::multiple,
                   "Text style"});
        auto& styles = *last_segmented_;
        for (std::size_t i = 0; i < styles.size(); ++i) {
            tip_anchors.push_back(std::make_unique<TipAnchor>(TipAnchor{[&styles, i] { return styles.segment_bounds(i); },
                                                                        [&styles, i] { return styles.focused_segment() == i; }}));
            live_tip([&styles, i] { return styles.segment_name(i); }, *tip_anchors.back(), false);
        }
        segmented(rows, "Disabled item",
                  {{{"Songs", std::nullopt, true}, {"Albums"}, {"Podcasts", std::nullopt, false, false}}, SegmentSelection::single, "Library"});
        auto& off = segmented(rows, "Disabled", {{{"Low"}, {"Medium", std::nullopt, true}, {"High"}}, SegmentSelection::single, "Quality"});
        off.set_enabled(false);
        auto column = layout::column(std::move(rows));
        column->spacing(inputs_layout.row_gap);
        return heading("Segmented buttons", column);
    }

    widgets::RangeSlider& range(std::vector<layout::NodePtr>& rows, const char* caption, widgets::RangeSliderOptions options) {
        auto& s = controls.emplace_with<widgets::RangeSlider>(
            [this, caption](widgets::RangeSlider& w, bool changed) {
                if (changed) {
                    ctx.report(std::string(caption) + ": " + w.value_text(widgets::RangeThumb::start) + " to " +
                               w.value_text(widgets::RangeThumb::end));
                }
            },
            std::move(options), ctx.fonts);
        focus_.add(s.thumb(widgets::RangeThumb::start));
        focus_.add(s.thumb(widgets::RangeThumb::end));
        enabler(caption, s);
        rows.push_back(captioned(caption, width_limited(widgets::node(s), inputs_layout.slider_width)));
        return s;
    }

    layout::NodePtr ranges() {
        const InputsLayout& l = inputs_layout;
        std::vector<layout::NodePtr> rows;
        range(rows, "Price", {.start = l.price_start, .end = l.price_end, .min = l.price_min, .max = l.price_max, .accessible_name = "Price",
                              .value_label = price_label});
        range(rows, "Steps", {.start = 0.2, .end = 0.7, .steps = l.tick_steps, .accessible_name = "Steps", .value_label = percent_label});
        range(rows, "Disabled", {.start = 0.3, .end = 0.6, .enabled = false, .accessible_name = "Disabled range", .value_label = percent_label});
        auto column = layout::column(std::move(rows));
        column->spacing(base_page_layout.heading_gap);
        return heading("Range sliders", column);
    }

    widgets::Slider& slider(widgets::SliderOptions options) {
        const std::string name = options.accessible_name;
        auto& s = controls.emplace_with<widgets::Slider>(
            [this, name](widgets::Slider& w, bool changed) {
                if (changed) {
                    ctx.report(name + ": " + (w.value_text().empty() ? percent_label(w.fraction()) : w.value_text()));
                }
            },
            std::move(options), ctx.fonts);
        focus_.add(s);
        enabler(name, s);
        return s;
    }

    layout::NodePtr sliders() {
        const InputsLayout& l = inputs_layout;
        std::vector<layout::NodePtr> rows;
        auto& centered = slider({.value = 12.0, .min = l.balance_min, .max = l.balance_max, .accessible_name = "Balance", .centered = true,
                                 .value_label = whole_label});
        rows.push_back(captioned("Centered", width_limited(widgets::node(centered), l.slider_width)));
        auto& corner = slider({.value = 0.6, .accessible_name = "Corner", .track_corner = l.custom_corner, .value_label = percent_label});
        rows.push_back(captioned("Track corner", width_limited(widgets::node(corner), l.slider_width)));
        auto& ticks = slider({.value = 0.4, .steps = 4, .accessible_name = "Ticks", .value_label = percent_label});
        rows.push_back(captioned("Indicator", width_limited(widgets::node(ticks), l.slider_width)));
        auto& down = slider({.value = 0.3, .accessible_name = "Top to bottom", .orientation = widgets::SliderOrientation::vertical});
        down.set_length(l.vertical_length);
        auto& up = slider({.value = 0.7, .accessible_name = "Bottom to top", .orientation = widgets::SliderOrientation::vertical,
                           .top_to_bottom = false});
        up.set_length(l.vertical_length);
        auto& centered_up = slider({.value = 0.0, .min = l.balance_min, .max = l.balance_max, .accessible_name = "Vertical balance",
                                    .orientation = widgets::SliderOrientation::vertical, .top_to_bottom = false, .centered = true});
        centered_up.set_length(l.vertical_length);
        auto verticals = items({widgets::centered_node(down), widgets::centered_node(up), widgets::centered_node(centered_up)});
        verticals->spacing(l.vertical_gap).line_spacing(l.vertical_gap);
        rows.push_back(captioned("Vertical", verticals));
        auto column = layout::column(std::move(rows));
        column->spacing(base_page_layout.heading_gap);
        return heading("Sliders", column);
    }

    widgets::SegmentedButtonRow* last_segmented_ = nullptr;
};

}

std::unique_ptr<Page> make_inputs_page(PageContext context) {
    return std::make_unique<InputsPage>(std::move(context));
}

}
