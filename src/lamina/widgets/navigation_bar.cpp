#include "lamina/style/style.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/navigation_bar.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

const tokens::NavigationBarTokens& t() {
    return current_style().navigation_bar;
}

double label_height(const text::TextLine& line) {
    return line.style.line_height;
}

}

NavigationBar::NavigationBar(NavigationBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)), fonts_(fonts), badges_(options_.items, fonts_), labels_(options_.labels ? 1.0 : 0.0, current_style().motion.default_spatial()) {
    name_navigation_items(options_.items);
    const auto style = current_style().type.text(t().label);
    for (std::size_t i = 0; i < options_.items.size(); ++i) {
        std::optional<text::TextLine> label;
        if (!options_.items[i].label.empty()) {
            label = text::layout_line(fonts_, options_.items[i].label, style);
        }
        const double on = i == options_.selected ? 1.0 : 0.0;
        items_.push_back({std::move(label), {}, motion::Animated(0.0, current_style().motion.fast_effects()),
                          motion::Animated(on, current_style().motion.default_spatial()), motion::Animated(on, current_style().motion.default_effects()),
                          Enablement(options_.items[i].enabled), options_.items[i].enabled});
    }
}

double NavigationBar::item_padding() const {
    const tokens::DenseLength& l = current_style().density.navigation_bar;
    return std::max(0.0, t().item_vertical_padding - (l.base - l.at(options_.density)) / 2.0);
}

double NavigationBar::labelled_height() const {
    double content = current_style().density.navigation_bar.at(options_.density);
    for (const Item& item : items_) {
        if (item.label) {
            content = std::max(content, 2.0 * item_padding() + t().indicator_height + t().indicator_to_label_space + label_height(*item.label));
        }
    }
    return content;
}

double NavigationBar::height() const {
    const double icon_only = std::max(t().icon_only_min_height, 2.0 * item_padding() + t().indicator_height);
    return motion::lerp(icon_only, labelled_height(), std::clamp(labels_.value(), 0.0, 1.0));
}

double NavigationBar::label_space(const Item& item) const {
    return item.label ? std::clamp(labels_.value(), 0.0, 1.0) * (t().indicator_to_label_space + label_height(*item.label)) : 0.0;
}

double NavigationBar::label_opacity() const {
    const double hide = std::clamp((labels_.value() - t().label_hide_progress) / (1.0 - t().label_hide_progress), 0.0, 1.0);
    return hide * hide;
}

void NavigationBar::set_labels(bool labels) {
    options_.labels = labels;
    labels_.animate_to(labels ? 1.0 : 0.0);
}

void NavigationBar::snap_to_targets() {
    labels_.snap_to(labels_.target());
    for (Item& item : items_) {
        for (motion::Animated* a : {&item.state_layer, &item.indicator, &item.color}) {
            a->snap_to(a->target());
        }
    }
}

render::Rect NavigationBar::bounds() const {
    return {origin_.x, origin_.y, width_, height()};
}

render::Rect NavigationBar::item_rect(std::size_t index) const {
    const double w = items_.empty() ? 0.0 : width_ / static_cast<double>(items_.size());
    return {origin_.x + w * static_cast<double>(index), origin_.y, w, height()};
}

double NavigationBar::content_top(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Item& item = items_[index];
    if (!item.label) {
        return r.y + (r.height - t().indicator_height) / 2.0;
    }
    const double content = 2.0 * item_padding() + t().indicator_height + label_space(item);
    return r.y + (r.height - content) / 2.0 + item_padding();
}

render::Rect NavigationBar::full_indicator_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    return {r.x + (r.width - t().indicator_width) / 2.0, content_top(index), t().indicator_width, t().indicator_height};
}

render::Rect NavigationBar::indicator_rect(std::size_t index) const {
    const render::Rect full = full_indicator_rect(index);
    const double w = t().indicator_width * std::max(0.0, items_[index].indicator.value());
    return {full.x + (full.width - w) / 2.0, full.y, w, full.height};
}

void NavigationBar::select(std::size_t index) {
    select_navigation_item(items_, options_.selected, index);
}

void NavigationBar::retarget_state_layers() {
    widgets::retarget_state_layers(items_, focus_.focused() ? std::optional<std::size_t>(focus_index_) : std::nullopt);
}

void NavigationBar::set_focused(bool focused) {
    focused = focused && can_focus();
    if (focused && !focus_.focused()) {
        focus_index_ = navigation_focus_start(items_, options_.selected);
    }
    focus_.set_focused(focused);
    retarget_state_layers();
}

void NavigationBar::refresh_items() {
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

void NavigationBar::set_item_enabled(std::size_t index, bool enabled) {
    if (index >= items_.size()) {
        return;
    }
    items_[index].allowed = enabled;
    refresh_items();
}

void NavigationBar::set_enabled(bool enabled) {
    row_enabled_ = enabled;
    refresh_items();
}

bool NavigationBar::handle(const input::KeyEvent& e) {
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
    if (const std::optional<std::size_t> target = item_key_target(e, focus_index_, items_.size(), ItemAxis::horizontal,
                                                                  [this](std::size_t i) { return item_enabled(i); })) {
        focus_index_ = *target;
        retarget_state_layers();
    }
    return false;
}

bool NavigationBar::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    const bool selected = select_on_click(
        items_, e, options_.selected, [this](std::size_t i) { return item_enabled(i) ? item_rect(i) : render::Rect{}; },
        [this](std::size_t i) { select(i); });
    retarget_state_layers();
    return selected;
}

void NavigationBar::cancel_interaction() {
    cancel_presses(items_);
    retarget_state_layers();
}

void NavigationBar::restyle() {
    style_stamp_.refresh();
    focus_.restyle();
    badges_.restyle();
    const auto& motion = current_style().motion;
    labels_.set_spec(motion.default_spatial());
    const auto style = current_style().type.text(t().label);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        Item& item = items_[i];
        if (item.label) {
            item.label = text::layout_line(fonts_, options_.items[i].label, style);
        }
        item.state_layer.set_spec(motion.fast_effects());
        item.indicator.set_spec(motion.default_spatial());
        item.color.set_spec(motion.default_effects());
        item.enabled.restyle();
    }
}

void NavigationBar::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    focus_.step(dt);
    labels_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.indicator.step(dt);
        item.color.step(dt);
        item.enabled.step(dt);
    }
}

bool NavigationBar::animating() const {
    return !focus_.settled() || !labels_.settled() || motion::any_unsettled(items_, &Item::state_layer, &Item::indicator, &Item::color, &Item::enabled);
}

void NavigationBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    canvas.fill_rect(bounds(), scheme[t().container]);
    const tokens::SelectionTokens& selection = current_style().selection;
    const bool surface = selection.mark == tokens::SelectionMark::surface;
    const color::Role active_icon = surface ? t().active_icon : selection.foreground;
    const color::Role active_label = surface ? t().active_label : selection.foreground;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const NavigationItem& spec = options_.items[i];
        const double progress = item.indicator.value();
        const render::Rect indicator = indicator_rect(i);
        if (surface && progress > 0.0 && indicator.width > 0.0) {
            canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(indicator.height / 2.0)),
                             color::with_alpha(scheme[t().indicator], std::clamp(progress, 0.0, 1.0)));
        }
        const render::Rect full = full_indicator_rect(i);
        if (item.state_layer.value() > 0.0) {
            canvas.fill_path(render::rounded_rect_path(full, render::uniform_radii(full.height / 2.0)),
                             color::with_alpha(scheme[t().state_layer], item.state_layer.value()));
        }
        const double k = item.color.value();
        icons::draw_centered(canvas, spec.icon, full, t().icon_size,
                             item.enabled.mix(resolve(scheme, t().disabled_icon), color::mix(scheme[t().inactive_icon], scheme[active_icon], k)),
                             icons::mix(spec.style, spec.selected_style, std::clamp(k, 0.0, 1.0)));
        badges_.paint(canvas, scheme, i,
                      {full.x + (full.width - t().icon_size) / 2.0, full.y + (full.height - t().icon_size) / 2.0, t().icon_size, t().icon_size},
                      item_rect(i));
        if (item.label && label_opacity() > 0.0) {
            const double x = full.x + (full.width - item.label->width) / 2.0;
            const color::Argb c = item.enabled.mix(resolve(scheme, t().disabled_label), color::mix(scheme[t().inactive_label], scheme[active_label], k));
            const double y = full.y + full.height + t().indicator_to_label_space;
            text::draw_line(canvas, fonts_, *item.label, {x, y}, color::scale_alpha(c, label_opacity()));
            if (!surface && selection.underline && progress > 0.0) {
                canvas.fill_rect({x, y + label_height(*item.label), item.label->width, selection.underline_thickness},
                                 color::scale_alpha(scheme[active_label], std::clamp(progress, 0.0, 1.0) * label_opacity()));
            }
        }
    }
    if (!items_.empty()) {
        const render::Rect pill = full_indicator_rect(focus_index_);
        focus_.paint(canvas, scheme, pill, pill.height / 2.0);
    }
}

}
