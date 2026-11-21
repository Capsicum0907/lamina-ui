#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/tabs.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::TabTokens& t = tokens::tab_tokens;
using tokens::motion_scheme;

constexpr motion::SpringSpec fade_in_spring = motion_scheme.default_effects();
constexpr motion::SpringSpec fade_out_spring = motion_scheme.fast_effects();

}

TabRow::TabRow(TabRowOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      indicator_offset_(0.0, motion_scheme.default_spatial()),
      indicator_width_(0.0, motion_scheme.default_spatial()) {
    const auto style = tokens::text_style(tokens::type_style(t.label));
    bool stacked = false;
    for (std::size_t i = 0; i < options_.tabs.size(); ++i) {
        TabItem& item = options_.tabs[i];
        if (item.label.empty()) {
            item.accessible_name = required_name(std::move(item.accessible_name), "A tab without a label");
        }
        std::optional<text::TextLine> label;
        if (!item.label.empty()) {
            label = text::layout_line(fonts_, item.label, style);
        }
        stacked = stacked || (label && item.icon && options_.icon_position == TabIconPosition::top);
        const double on = i == options_.selected ? 1.0 : 0.0;
        tabs_.push_back({std::move(label), item.icon, {}, motion::Animated(0.0, motion_scheme.fast_effects()),
                         motion::Animated(on, fade_in_spring), Enablement(item.enabled), item.enabled});
    }
    stacked_ = stacked;
    update_height();
}

void TabRow::update_height() {
    height_ = (stacked_ ? tokens::density_tokens.tab_with_icon : tokens::density_tokens.tab).at(options_.density);
}

void TabRow::set_density(tokens::Density density) {
    options_.density = density;
    update_height();
}

void TabRow::set_width(double width) {
    width_ = width;
    retarget_indicator();
}

render::Rect TabRow::bounds() const {
    return {origin_.x, origin_.y, width_, height_};
}

double TabRow::tab_width() const {
    return tabs_.empty() ? 0.0 : width_ / static_cast<double>(tabs_.size());
}

render::Rect TabRow::tab_rect(std::size_t index) const {
    const double w = tab_width();
    return {origin_.x + w * static_cast<double>(index), origin_.y, w, height_};
}

double TabRow::content_width(const Tab& tab) const {
    const double padding = 2.0 * t.horizontal_text_padding;
    const double text_width = tab.label ? tab.label->width : 0.0;
    const double icon_width = tab.icon ? t.icon_size : 0.0;
    double intrinsic = 0.0;
    if (options_.icon_position == TabIconPosition::leading && tab.icon && tab.label) {
        intrinsic = padding + icon_width + t.leading_icon_text_space + text_width;
    } else {
        intrinsic = std::max(tab.label ? text_width + padding : 0.0, icon_width);
    }
    return std::max(std::min(intrinsic, tab_width()) - padding, t.indicator_min_width);
}

double TabRow::indicator_target_width() const {
    if (tabs_.empty()) {
        return 0.0;
    }
    if (tokens::tab_style(options_.style).indicator_matches_content) {
        return content_width(tabs_[options_.selected]);
    }
    return tab_width();
}

void TabRow::retarget_indicator() {
    const double offset = tab_width() * static_cast<double>(options_.selected);
    const double width = indicator_target_width();
    if (!placed_) {
        indicator_offset_.snap_to(offset);
        indicator_width_.snap_to(width);
        placed_ = true;
        return;
    }
    indicator_offset_.animate_to(offset);
    indicator_width_.animate_to(width);
}

void TabRow::select(std::size_t index) {
    if (index >= tabs_.size() || index == options_.selected) {
        return;
    }
    Tab& old_tab = tabs_[options_.selected];
    old_tab.selected.set_spec(fade_out_spring);
    old_tab.selected.animate_to(0.0);
    Tab& new_tab = tabs_[index];
    new_tab.selected.set_spec(fade_in_spring);
    new_tab.selected.animate_to(1.0);
    options_.selected = index;
    retarget_indicator();
}

render::Rect TabRow::indicator_rect() const {
    const auto& style = tokens::tab_style(options_.style);
    const double w = std::max(0.0, indicator_width_.value());
    const double x = origin_.x + indicator_offset_.value() + (tab_width() - w) / 2.0;
    return {x, origin_.y + height_ - style.indicator_height, w, style.indicator_height};
}

void TabRow::retarget_state_layers() {
    widgets::retarget_state_layers(tabs_, focus_.focused() ? std::optional<std::size_t>(options_.selected) : std::nullopt);
}

void TabRow::set_focused(bool focused) {
    focus_.set_focused(focused && can_focus());
    retarget_state_layers();
}

bool TabRow::can_focus() const {
    return row_enabled_ && any_item_enabled(tabs_);
}

void TabRow::refresh_tabs() {
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        refresh_item(tabs_, i, row_enabled_);
    }
    if (!can_focus()) {
        focus_.set_focused(false);
    }
    retarget_state_layers();
}

void TabRow::set_tab_enabled(std::size_t index, bool enabled) {
    if (index >= tabs_.size()) {
        return;
    }
    tabs_[index].allowed = enabled;
    refresh_tabs();
}

AccessibleNode TabRow::accessible() const {
    AccessibleNode row{AccessibleRole::tab_list, options_.accessible_name, bounds(),
                       {.enabled = enabled(), .focusable = can_focus(), .focused = focused()}};
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        const TabItem& item = options_.tabs[i];
        const bool on = i == options_.selected;
        row.children.push_back({AccessibleRole::tab, item.accessible_name.empty() ? item.label : item.accessible_name, tab_rect(i),
                                {.enabled = tab_enabled(i), .focused = focused() && on, .selected = on}});
    }
    return row;
}

void TabRow::set_enabled(bool enabled) {
    row_enabled_ = enabled;
    refresh_tabs();
}

bool TabRow::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool selected = select_on_click(
        tabs_, e, options_.selected, [this](std::size_t i) { return tab_enabled(i) ? tab_rect(i) : render::Rect{}; },
        [this](std::size_t i) { select(i); });
    retarget_state_layers();
    return selected;
}

bool TabRow::handle(const input::KeyEvent& e) {
    if (!focus_.focused()) {
        return false;
    }
    const std::optional<std::size_t> target = item_key_target(e, options_.selected, tabs_.size(), ItemAxis::horizontal,
                                                              [this](std::size_t i) { return tab_enabled(i); });
    if (!target || *target == options_.selected) {
        return false;
    }
    select(*target);
    retarget_state_layers();
    return true;
}

void TabRow::cancel_interaction() {
    cancel_presses(tabs_);
    retarget_state_layers();
}

void TabRow::step(double dt) {
    indicator_offset_.step(dt);
    indicator_width_.step(dt);
    focus_.step(dt);
    for (Tab& tab : tabs_) {
        tab.state_layer.step(dt);
        tab.selected.step(dt);
        tab.enabled.step(dt);
    }
}

bool TabRow::animating() const {
    if (!indicator_offset_.settled() || !indicator_width_.settled() || !focus_.settled()) {
        return true;
    }
    return motion::any_unsettled(tabs_, &Tab::state_layer, &Tab::selected, &Tab::enabled);
}

void TabRow::paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const {
    const auto& style = tokens::tab_style(options_.style);
    const color::Argb active = scheme[style.active_content];
    if (tab.state_layer.value() > 0.0) {
        canvas.fill_rect(r, color::with_alpha(active, tab.state_layer.value()));
    }
    const color::Argb content =
        color::scale_alpha(color::mix(scheme[style.inactive_content], active, tab.selected.value()),
                           motion::lerp(t.disabled_content_opacity, 1.0, tab.enabled.amount()));
    const double cx = r.x + r.width / 2.0;
    const double cy = r.y + r.height / 2.0;

    if (tab.label && tab.icon && options_.icon_position == TabIconPosition::top) {
        const double removed = t.icon_and_label_height - r.height;
        const double baseline = r.y + r.height - (t.text_baseline_with_icon - removed / 2.0) - style.indicator_height;
        const double icon_top = baseline - t.icon_distance_from_baseline - t.icon_size;
        tab.icon->font.draw(canvas, tab.icon->symbol, {cx - t.icon_size / 2.0, icon_top}, t.icon_size, content);
        text::draw_line(canvas, fonts_, *tab.label, {cx - tab.label->width / 2.0, baseline - tab.label->baseline}, content);
        return;
    }
    if (tab.label && tab.icon) {
        const double total = t.icon_size + t.leading_icon_text_space + tab.label->width;
        const double x = cx - total / 2.0;
        tab.icon->font.draw(canvas, tab.icon->symbol, {x, cy - t.icon_size / 2.0}, t.icon_size, content);
        text::draw_line(canvas, fonts_, *tab.label, {x + t.icon_size + t.leading_icon_text_space, cy - tab.label->box_height() / 2.0},
                        content);
        return;
    }
    if (tab.icon) {
        icons::draw_centered(canvas, *tab.icon, render::Point{cx, cy}, t.icon_size, content);
    }
    if (tab.label) {
        text::draw_line(canvas, fonts_, *tab.label, {cx - tab.label->width / 2.0, cy - tab.label->box_height() / 2.0}, content);
    }
}

void TabRow::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = tokens::tab_style(options_.style);
    const render::Rect r = bounds();
    canvas.fill_rect(r, scheme[style.container]);
    canvas.push_clip(r);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        paint_tab(canvas, scheme, tabs_[i], tab_rect(i));
    }
    canvas.pop_clip();
    const render::Rect divider{r.x, r.y + r.height - t.divider_thickness, r.width, t.divider_thickness};
    canvas.fill_rect(divider, scheme[t.divider]);
    if (!tabs_.empty()) {
        focus_.paint_inward(canvas, scheme, tab_rect(options_.selected), t.focus_ring_corner);
    }
    const render::Rect indicator = indicator_rect();
    canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(style.indicator_corner)), scheme[style.indicator]);
}

}
