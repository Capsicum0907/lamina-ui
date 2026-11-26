#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/tabs.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/tokens/scroll_area.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

const tokens::TabTokens& t() {
    return current_style().tabs.common;
}

motion::SpringSpec fade_in_spring() {
    return current_style().motion.default_effects();
}
motion::SpringSpec fade_out_spring() {
    return current_style().motion.fast_effects();
}

}

TabRow::TabRow(TabRowOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      indicator_offset_(0.0, current_style().motion.default_spatial()),
      indicator_width_(0.0, current_style().motion.default_spatial()),
      scroll_(0.0, current_style().motion.default_spatial()),
      fling_(fling_decay()) {
    const auto style = current_style().type.text(t().label);
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
        tabs_.push_back({std::move(label), item.icon, {}, motion::Animated(0.0, current_style().motion.fast_effects()),
                         motion::Animated(on, fade_in_spring()), Enablement(item.enabled), item.enabled});
    }
    stacked_ = stacked;
    for (const Tab& tab : tabs_) {
        natural_widths_.push_back(natural_width(tab));
    }
    update_height();
}

void TabRow::update_height() {
    height_ = (stacked_ ? current_style().density.tab_with_icon : current_style().density.tab).at(options_.density);
}

void TabRow::set_density(tokens::Density density) {
    options_.density = density;
    update_height();
}

void TabRow::set_width(double width) {
    const bool first = !placed_;
    width_ = width;
    retarget_indicator();
    if (first && options_.scrollable && !tabs_.empty()) {
        scroll_.snap_to(centred_scroll(options_.selected));
    }
    clamp_scroll();
}

double TabRow::scroll_extent() const {
    if (!options_.scrollable) {
        return width_;
    }
    double sum = 2.0 * options_.edge_padding;
    for (double w : natural_widths_) {
        sum += w;
    }
    return sum;
}

double TabRow::max_scroll() const {
    return options_.scrollable ? std::max(0.0, scroll_extent() - width_) : 0.0;
}

double TabRow::centred_scroll(std::size_t index) const {
    const double target = slot_start(index) + slot_width(index) / 2.0 - width_ / 2.0;
    return std::clamp(target, 0.0, max_scroll());
}

void TabRow::clamp_scroll() {
    const double top = max_scroll();
    scroll_.animate_to(std::clamp(scroll_.target(), 0.0, top));
    if (scroll_.value() > top || scroll_.value() < 0.0) {
        scroll_.snap_to(std::clamp(scroll_.value(), 0.0, top));
    }
}

render::Rect TabRow::bounds() const {
    return {origin_.x, origin_.y, width_, height_};
}

double TabRow::tab_width() const {
    return tabs_.empty() ? 0.0 : width_ / static_cast<double>(tabs_.size());
}

double TabRow::slot_start(std::size_t index) const {
    if (!options_.scrollable) {
        return tab_width() * static_cast<double>(index);
    }
    double x = options_.edge_padding;
    for (std::size_t i = 0; i < index; ++i) {
        x += natural_widths_[i];
    }
    return x;
}

double TabRow::slot_width(std::size_t index) const {
    return options_.scrollable ? natural_widths_[index] : tab_width();
}

render::Rect TabRow::tab_rect(std::size_t index) const {
    return {origin_.x + slot_start(index) - scroll_.value(), origin_.y, slot_width(index), height_};
}

double TabRow::natural_width(const Tab& tab) const {
    const double padding = 2.0 * t().horizontal_text_padding;
    const double text_width = tab.label ? tab.label->width : 0.0;
    const double icon_width = tab.icon ? t().icon_size : 0.0;
    double intrinsic = 0.0;
    if (options_.icon_position == TabIconPosition::leading && tab.icon && tab.label) {
        intrinsic = padding + icon_width + t().leading_icon_text_space + text_width;
    } else {
        intrinsic = std::max(tab.label ? text_width + padding : 0.0, icon_width + padding);
    }
    return std::max(intrinsic, t().scrollable_min_tab_width);
}

double TabRow::content_width(const Tab& tab, double slot) const {
    const double padding = 2.0 * t().horizontal_text_padding;
    const double text_width = tab.label ? tab.label->width : 0.0;
    const double icon_width = tab.icon ? t().icon_size : 0.0;
    double intrinsic = 0.0;
    if (options_.icon_position == TabIconPosition::leading && tab.icon && tab.label) {
        intrinsic = padding + icon_width + t().leading_icon_text_space + text_width;
    } else {
        intrinsic = std::max(tab.label ? text_width + padding : 0.0, icon_width);
    }
    return std::max(std::min(intrinsic, slot) - padding, t().indicator_min_width);
}

double TabRow::indicator_width_for(std::size_t index) const {
    if (current_style().tabs.style(options_.style).indicator_matches_content) {
        return content_width(tabs_[index], slot_width(index));
    }
    return slot_width(index);
}

double TabRow::indicator_target_width() const {
    return tabs_.empty() ? 0.0 : indicator_width_for(options_.selected);
}

void TabRow::set_indicator_position(double position) {
    if (tabs_.empty()) {
        return;
    }
    const double last = static_cast<double>(tabs_.size() - 1);
    const double p = std::clamp(position, 0.0, last);
    const auto from = static_cast<std::size_t>(std::floor(p));
    const std::size_t to = std::min(from + 1, tabs_.size() - 1);
    const double f = p - static_cast<double>(from);
    const double centre_from = slot_start(from) + slot_width(from) / 2.0;
    const double centre_to = slot_start(to) + slot_width(to) / 2.0;
    indicator_offset_.snap_to(motion::lerp(centre_from, centre_to, f));
    indicator_width_.snap_to(motion::lerp(indicator_width_for(from), indicator_width_for(to), f));
    placed_ = true;
}

void TabRow::retarget_indicator() {
    if (tabs_.empty()) {
        return;
    }
    const double offset = slot_start(options_.selected) + slot_width(options_.selected) / 2.0;
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
    old_tab.selected.set_spec(fade_out_spring());
    old_tab.selected.animate_to(0.0);
    Tab& new_tab = tabs_[index];
    new_tab.selected.set_spec(fade_in_spring());
    new_tab.selected.animate_to(1.0);
    options_.selected = index;
    retarget_indicator();
    if (options_.scrollable) {
        fling_.stop();
        scroll_.animate_to(centred_scroll(index));
    }
}

render::Rect TabRow::indicator_rect() const {
    const auto& style = current_style().tabs.style(options_.style);
    const double w = std::max(0.0, indicator_width_.value());
    const double x = origin_.x + indicator_offset_.value() - scroll_.value() - w / 2.0;
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

render::Rect TabRow::visible_rect(const render::Rect& r) const {
    const render::Rect b = bounds();
    const double left = std::max(r.x, b.x);
    const double right = std::min(r.x + r.width, b.x + b.width);
    return right > left ? render::Rect{left, r.y, right - left, r.height} : render::Rect{};
}

AccessibleNode TabRow::accessible() const {
    AccessibleNode row{AccessibleRole::tab_list, options_.accessible_name, bounds(),
                       {.enabled = enabled(), .focusable = can_focus(), .focused = focused()}};
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        const TabItem& item = options_.tabs[i];
        const bool on = i == options_.selected;
        row.children.push_back({AccessibleRole::tab, item.accessible_name.empty() ? item.label : item.accessible_name, visible_rect(tab_rect(i)),
                                {.enabled = tab_enabled(i), .focused = focused() && on, .selected = on}});
    }
    return row;
}

void TabRow::set_enabled(bool enabled) {
    row_enabled_ = enabled;
    refresh_tabs();
}

void TabRow::follow_touch(double dx) {
    scroll_.snap_to(std::clamp(scroll_.value() - dx, 0.0, max_scroll()));
}

std::optional<bool> TabRow::handle_touch(const input::PointerEvent& e) {
    switch (e.action) {
    case input::PointerAction::down: {
        const bool inside = options_.scrollable && max_scroll() > 0.0 && contains(bounds(), e.x_dp, e.y_dp);
        const bool caught = inside && fling_.active();
        fling_.stop();
        if (touch_.press(e, {inside, false}, caught).step == TouchScrollStep::caught) {
            cancel_presses(tabs_);
            retarget_state_layers();
            return false;
        }
        return std::nullopt;
    }
    case input::PointerAction::move:
        if (touch_.owns()) {
            follow_touch(touch_.move(e).dx);
            return false;
        }
        return std::nullopt;
    case input::PointerAction::up:
        if (touch_.owns()) {
            const TouchScrollUpdate u = touch_.release(e);
            follow_touch(u.dx);
            if (std::abs(u.velocity_x) > touch_.tokens().minimum_fling_velocity) {
                fling_.start(scroll_.value(), -u.velocity_x);
            }
            return false;
        }
        touch_.cancel();
        return std::nullopt;
    case input::PointerAction::leave:
        break;
    }
    touch_.cancel();
    return std::nullopt;
}

bool TabRow::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (e.kind == input::PointerKind::touch) {
        if (const std::optional<bool> r = handle_touch(e)) {
            return *r;
        }
    }
    const bool selected = select_on_click(
        tabs_, e, options_.selected, [this](std::size_t i) { return tab_enabled(i) ? visible_rect(tab_rect(i)) : render::Rect{}; },
        [this](std::size_t i) { select(i); });
    retarget_state_layers();
    if (e.action == input::PointerAction::move && touch_.pending()) {
        const TouchScrollUpdate u = touch_.move(e);
        if (u.step == TouchScrollStep::claim) {
            cancel_presses(tabs_);
            retarget_state_layers();
            follow_touch(u.dx);
        }
    }
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

bool TabRow::handle(const input::WheelEvent& e) {
    fling_.stop();
    if (!options_.scrollable || max_scroll() <= 0.0 || !contains(bounds(), e.x_dp, e.y_dp)) {
        return false;
    }
    const double notches = e.dx != 0.0 ? e.dx : e.dy;
    if (notches == 0.0) {
        return false;
    }
    scroll_.animate_to(std::clamp(scroll_.target() + notches * current_style().scroll_area.wheel_step, 0.0, max_scroll()));
    return true;
}

void TabRow::cancel_interaction() {
    touch_.cancel();
    fling_.stop();
    cancel_presses(tabs_);
    retarget_state_layers();
}

void TabRow::restyle() {
    style_stamp_.refresh();
    const bool centred = placed_ && options_.scrollable && !tabs_.empty() && scroll_.target() == centred_scroll(options_.selected);
    const motion::SpringSpec spatial = current_style().motion.default_spatial();
    indicator_offset_.set_spec(spatial);
    indicator_width_.set_spec(spatial);
    scroll_.set_spec(spatial);
    touch_.set_tokens(current_style().gesture.touch_scroll);
    fling_.set_decay(fling_decay());
    focus_.restyle();
    const auto style = current_style().type.text(t().label);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        Tab& tab = tabs_[i];
        if (tab.label) {
            tab.label = text::layout_line(fonts_, options_.tabs[i].label, style);
        }
        tab.state_layer.set_spec(current_style().motion.fast_effects());
        tab.selected.set_spec(tab.selected.target() > 0.5 ? fade_in_spring() : fade_out_spring());
        tab.enabled.restyle();
        natural_widths_[i] = natural_width(tab);
    }
    update_height();
    if (placed_) {
        retarget_indicator();
        if (centred) {
            scroll_.animate_to(centred_scroll(options_.selected));
        }
        clamp_scroll();
    }
}

void TabRow::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    indicator_offset_.step(dt);
    indicator_width_.step(dt);
    if (fling_.active()) {
        const double v = fling_.step(dt);
        follow_touch(scroll_.value() - v);
        if (std::abs(v - scroll_.value()) > touch_.tokens().edge_tolerance) {
            fling_.stop();
        }
    }
    scroll_.step(dt);
    focus_.step(dt);
    for (Tab& tab : tabs_) {
        tab.state_layer.step(dt);
        tab.selected.step(dt);
        tab.enabled.step(dt);
    }
}

bool TabRow::animating() const {
    if (fling_.active() || !indicator_offset_.settled() || !indicator_width_.settled() || !scroll_.settled() || !focus_.settled()) {
        return true;
    }
    return motion::any_unsettled(tabs_, &Tab::state_layer, &Tab::selected, &Tab::enabled);
}

void TabRow::paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const {
    const auto& style = current_style().tabs.style(options_.style);
    const color::Argb active = scheme[style.active_content];
    if (tab.state_layer.value() > 0.0) {
        canvas.fill_rect(r, color::with_alpha(active, tab.state_layer.value()));
    }
    const color::Argb content =
        color::scale_alpha(color::mix(scheme[style.inactive_content], active, tab.selected.value()),
                           motion::lerp(t().disabled_content_opacity, 1.0, tab.enabled.amount()));
    const double cx = r.x + r.width / 2.0;
    const double cy = r.y + r.height / 2.0;

    if (tab.label && tab.icon && options_.icon_position == TabIconPosition::top) {
        const double removed = t().icon_and_label_height - r.height;
        const double baseline = r.y + r.height - (t().text_baseline_with_icon - removed / 2.0) - style.indicator_height;
        const double icon_top = baseline - t().icon_distance_from_baseline - t().icon_size;
        tab.icon->font.draw(canvas, tab.icon->symbol, {cx - t().icon_size / 2.0, icon_top}, t().icon_size, content);
        text::draw_line(canvas, fonts_, *tab.label, {cx - tab.label->width / 2.0, baseline - tab.label->baseline}, content);
        return;
    }
    if (tab.label && tab.icon) {
        const double total = t().icon_size + t().leading_icon_text_space + tab.label->width;
        const double x = cx - total / 2.0;
        tab.icon->font.draw(canvas, tab.icon->symbol, {x, cy - t().icon_size / 2.0}, t().icon_size, content);
        text::draw_line(canvas, fonts_, *tab.label, {x + t().icon_size + t().leading_icon_text_space, cy - tab.label->box_height() / 2.0},
                        content);
        return;
    }
    if (tab.icon) {
        icons::draw_centered(canvas, *tab.icon, render::Point{cx, cy}, t().icon_size, content);
    }
    if (tab.label) {
        text::draw_line(canvas, fonts_, *tab.label, {cx - tab.label->width / 2.0, cy - tab.label->box_height() / 2.0}, content);
    }
}

void TabRow::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const auto& style = current_style().tabs.style(options_.style);
    const render::Rect r = bounds();
    canvas.fill_rect(r, scheme[style.container]);
    canvas.push_clip(r);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        paint_tab(canvas, scheme, tabs_[i], tab_rect(i));
    }
    canvas.pop_clip();
    const render::Rect divider{r.x, r.y + r.height - t().divider_thickness, r.width, t().divider_thickness};
    canvas.fill_rect(divider, scheme[t().divider]);
    canvas.push_clip(r);
    if (!tabs_.empty()) {
        focus_.paint_inward(canvas, scheme, tab_rect(options_.selected), t().focus_ring_corner);
    }
    const render::Rect indicator = indicator_rect();
    canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(style.indicator_corner)), scheme[style.indicator]);
    canvas.pop_clip();
}

}
