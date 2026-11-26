#include "lamina/style/style.hpp"
#include "lamina/widgets/side_navigation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/side_navigation.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

const tokens::SideNavigationTokens& t() {
    return current_style().side_navigation;
}

double unit(double v) {
    return std::clamp(v, 0.0, 1.0);
}

double now(const motion::Animated& a) {
    return unit(a.settled() ? a.target() : a.value());
}

}

SideNavigation::SideNavigation(SideNavigationOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      badges_(options_.items, fonts_),
      expansion_(options_.expanded ? 1.0 : 0.0, t().expansion),
      drawer_(options_.drawer ? 1.0 : 0.0, t().expansion),
      max_width_(std::numeric_limits<double>::infinity()) {
    const auto style = current_style().type.text(t().label);
    for (std::size_t i = 0; i < options_.items.size(); ++i) {
        const double on = i == options_.selected ? 1.0 : 0.0;
        items_.push_back({text::layout_line(fonts_, options_.items[i].label, style), {}, motion::Animated(0.0, t().state_layer_spring),
                          motion::Animated(on, t().selection), motion::Animated(on, t().colors), Enablement(options_.items[i].enabled),
                          options_.items[i].enabled});
    }
    update_width();
}

double SideNavigation::expansion() const {
    return now(expansion_);
}

double SideNavigation::drawer_amount() const {
    return now(drawer_);
}

double SideNavigation::label_opacity() const {
    const double a = unit((expansion() - t().label_fade_start) / (1.0 - t().label_fade_start));
    return a * a;
}

double SideNavigation::row_height() const {
    return current_style().density.drawer_item.at(options_.density);
}

double SideNavigation::hug_width(std::size_t index) const {
    return 2.0 * t().indicator_padding + t().icon_size + t().icon_label_space + items_[index].label.width;
}

double SideNavigation::rail_width() const {
    double widest = 0.0;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        widest = std::max(widest, hug_width(i) + 2.0 * t().item_inset);
    }
    return std::min(std::clamp(widest, t().expanded_min_width, t().expanded_max_width), max_width_);
}

double SideNavigation::drawer_width() const {
    return std::min(t().drawer_width, max_width_);
}

double SideNavigation::width_at(double expansion, double drawer) const {
    const double open = motion::lerp(rail_width(), drawer_width(), unit(drawer));
    return std::min(motion::lerp(t().collapsed_width, open, unit(expansion)), max_width_);
}

void SideNavigation::update_width() {
    width_ = width_at(expansion(), drawer_amount());
}

layout::Size SideNavigation::measure(const layout::Constraints& c) {
    max_width_ = std::isfinite(c.max_width) ? c.max_width : std::numeric_limits<double>::infinity();
    update_width();
    const double content = t().top_space + row_height() * static_cast<double>(items_.size());
    return {width_, std::isfinite(c.max_height) ? c.max_height : content};
}

void SideNavigation::arrange(const render::Rect& rect) {
    origin_ = {rect.x, rect.y};
    height_ = rect.height;
}

void SideNavigation::set_expanded(bool expanded) {
    options_.expanded = expanded;
    expansion_.animate_to(expanded ? 1.0 : 0.0);
}

void SideNavigation::set_drawer(bool drawer) {
    options_.drawer = drawer;
    drawer_.animate_to(drawer ? 1.0 : 0.0);
}

void SideNavigation::snap_to_targets() {
    for (motion::Animated* a : {&expansion_, &drawer_}) {
        a->snap_to(a->target());
    }
    for (Item& item : items_) {
        for (motion::Animated* a : {&item.state_layer, &item.indicator, &item.color}) {
            a->snap_to(a->target());
        }
    }
    update_width();
}

void SideNavigation::set_density(tokens::Density density) {
    options_.density = density;
}

void SideNavigation::select(std::size_t index) {
    select_navigation_item(items_, options_.selected, index);
}

double SideNavigation::row_top(std::size_t index) const {
    return origin_.y + t().top_space + row_height() * static_cast<double>(index);
}

render::Rect SideNavigation::item_rect(std::size_t index) const {
    return {origin_.x, row_top(index), width_, row_height()};
}

render::Rect SideNavigation::shape_rect(std::size_t index) const {
    const double e = expansion();
    const double d = drawer_amount();
    const double row = motion::lerp(rail_width(), drawer_width(), d) - 2.0 * t().item_inset;
    const double open = motion::lerp(std::min(hug_width(index), row), row, d);
    const double w = motion::lerp(t().collapsed_indicator_width, open, e);
    const double h = motion::lerp(t().collapsed_indicator_height, row_height(), e);
    return {origin_.x + t().item_inset, row_top(index) + (row_height() - h) / 2.0, std::max(0.0, w), h};
}

render::Rect SideNavigation::indicator_rect(std::size_t index) const {
    const render::Rect s = shape_rect(index);
    return {s.x, s.y, s.width * std::max(0.0, items_[index].indicator.value()), s.height};
}

render::Rect SideNavigation::icon_rect(std::size_t index) const {
    return {origin_.x + t().item_inset + t().indicator_padding, row_top(index) + (row_height() - t().icon_size) / 2.0, t().icon_size, t().icon_size};
}

render::Rect SideNavigation::label_rect(std::size_t index) const {
    const render::Rect icon = icon_rect(index);
    const text::TextLine& line = items_[index].label;
    const double h = line.style.line_height;
    return {icon.x + icon.width + t().icon_label_space, row_top(index) + (row_height() - h) / 2.0, line.width, h};
}

AccessibleNode SideNavigation::accessible() const {
    return navigation_accessible(options_.items, items_, options_.accessible_name, bounds(), options_.selected, focused(), focus_index_,
                                 [this](std::size_t i) { return item_rect(i); });
}

void SideNavigation::retarget_state_layers() {
    widgets::retarget_state_layers(items_, focus_.focused() ? std::optional<std::size_t>(focus_index_) : std::nullopt);
}

void SideNavigation::set_focused(bool focused) {
    focused = focused && can_focus();
    if (focused && !focus_.focused()) {
        focus_index_ = navigation_focus_start(items_, options_.selected);
    }
    focus_.set_focused(focused);
    retarget_state_layers();
}

void SideNavigation::refresh_items() {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        refresh_item(items_, i, row_enabled_);
    }
    if (!can_focus()) {
        focus_.set_focused(false);
    } else if (!items_[focus_index_].enabled.enabled()) {
        focus_index_ = navigation_focus_start(items_, focus_index_);
    }
    retarget_state_layers();
}

void SideNavigation::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= items_.size()) {
        return;
    }
    items_[index].allowed = enabled;
    refresh_items();
}

void SideNavigation::set_enabled(bool enabled) {
    row_enabled_ = enabled;
    refresh_items();
}

bool SideNavigation::handle(const input::KeyEvent& e) {
    if (!focus_.focused()) {
        return false;
    }
    if (focus_.activates(e, KeyActivation::space_or_enter)) {
        if (focus_index_ == options_.selected || !item_enabled(focus_index_)) {
            return false;
        }
        select(focus_index_);
        retarget_state_layers();
        return true;
    }
    if (const std::optional<std::size_t> target = item_key_target(e, focus_index_, items_.size(), ItemAxis::vertical,
                                                                  [this](std::size_t i) { return item_enabled(i); })) {
        focus_index_ = *target;
        retarget_state_layers();
    }
    return false;
}

bool SideNavigation::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool selected = select_on_click(
        items_, e, options_.selected, [this](std::size_t i) { return item_enabled(i) ? item_rect(i) : render::Rect{}; },
        [this](std::size_t i) { select(i); });
    retarget_state_layers();
    return selected;
}

void SideNavigation::cancel_interaction() {
    cancel_presses(items_);
    retarget_state_layers();
}

void SideNavigation::restyle() {
    style_stamp_.refresh();
    focus_.restyle();
    badges_.restyle();
    expansion_.set_spec(t().expansion);
    drawer_.set_spec(t().expansion);
    const auto style = current_style().type.text(t().label);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        Item& item = items_[i];
        item.label = text::layout_line(fonts_, options_.items[i].label, style);
        item.state_layer.set_spec(t().state_layer_spring);
        item.indicator.set_spec(t().selection);
        item.color.set_spec(t().colors);
        item.enabled.restyle();
    }
    update_width();
}

void SideNavigation::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    focus_.step(dt);
    expansion_.step(dt);
    drawer_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.indicator.step(dt);
        item.color.step(dt);
        item.enabled.step(dt);
    }
    update_width();
}

bool SideNavigation::animating() const {
    if (!expansion_.settled() || !drawer_.settled() || !focus_.settled()) {
        return true;
    }
    return motion::any_unsettled(items_, &Item::state_layer, &Item::indicator, &Item::color, &Item::enabled);
}

void SideNavigation::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = bounds();
    canvas.fill_rect(b, scheme[t().container]);
    canvas.push_clip(b);
    const double labels = label_opacity();
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const NavigationItem& spec = options_.items[i];
        const double progress = unit(item.indicator.value());
        const render::Rect indicator = indicator_rect(i);
        if (progress > 0.0 && indicator.width > 0.0) {
            canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(std::min(indicator.width, indicator.height) / 2.0)),
                             color::with_alpha(scheme[t().indicator], progress));
        }
        const render::Rect shape = shape_rect(i);
        if (item.state_layer.value() > 0.0) {
            canvas.fill_path(render::rounded_rect_path(shape, render::uniform_radii(std::min(shape.width, shape.height) / 2.0)),
                             color::with_alpha(scheme[t().state_layer], item.state_layer.value()));
        }
        const double k = unit(item.color.value());
        const render::Rect icon = icon_rect(i);
        spec.icon.font.draw(canvas, spec.icon.symbol, {icon.x, icon.y}, t().icon_size,
                            item.enabled.mix(resolve(scheme, t().disabled_icon), color::mix(scheme[t().inactive_icon], scheme[t().active_icon], k)),
                            icons::mix(spec.style, spec.selected_style, k));
        badges_.paint(canvas, scheme, i, icon, item_rect(i));
        if (labels > 0.0) {
            const render::Rect label = label_rect(i);
            const render::Rect row = item_rect(i);
            const double end = row.x + row.width - t().item_inset;
            const text::TextLine& line = item.label;
            const color::Argb c = item.enabled.mix(resolve(scheme, t().disabled_label), color::mix(scheme[t().inactive_label], scheme[t().active_label], k));
            canvas.push_clip({label.x, row.y, std::max(0.0, end - label.x), row.height});
            text::draw_line(canvas, fonts_, line, {label.x, label.y + (label.height - line.box_height()) / 2.0}, color::scale_alpha(c, labels));
            canvas.pop_clip();
        }
    }
    if (!items_.empty()) {
        const render::Rect ring = shape_rect(focus_index_);
        focus_.paint(canvas, scheme, ring, std::min(ring.width, ring.height) / 2.0);
    }
    canvas.pop_clip();
}

}
