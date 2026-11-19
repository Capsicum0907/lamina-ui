#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/navigation_rail.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::NavigationRailTokens& t = tokens::navigation_rail_tokens;
using tokens::motion_scheme;

}

NavigationRail::NavigationRail(NavigationRailOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      min_width_(options_.expanded ? t.expanded_min_width : t.collapsed_width, motion_scheme.default_spatial()),
      full_width_(options_.expanded ? t.expanded_max_width : t.collapsed_width, motion_scheme.default_spatial()),
      item_space_(options_.expanded ? t.expanded_item_space : t.collapsed_item_space, motion_scheme.default_spatial()),
      item_min_height_(options_.expanded ? t.expanded_item_min_height : t.collapsed_item_min_height, motion_scheme.default_spatial()),
      max_width_(t.expanded_max_width) {
    const auto top_style = tokens::text_style(tokens::type_style(t.top_label));
    const auto start_style = tokens::text_style(tokens::type_style(t.start_label));
    const double mode = options_.expanded ? 1.0 : 0.0;
    for (std::size_t i = 0; i < options_.items.size(); ++i) {
        const double on = i == options_.selected ? 1.0 : 0.0;
        items_.push_back({text::layout_line(fonts_, options_.items[i].label, top_style),
                          text::layout_line(fonts_, options_.items[i].label, start_style), {},
                          motion::Animated(0.0, motion_scheme.fast_effects()),
                          motion::Animated(on, motion_scheme.default_spatial()), motion::Animated(mode, motion_scheme.default_spatial()),
                          motion::Animated(on, motion_scheme.default_effects()), motion::Animated(mode, motion_scheme.default_effects())});
    }
    update_width();
}

NavigationRail::Geometry NavigationRail::geometry(std::size_t index, double item_max_width) const {
    const Item& item = items_[index];
    const bool top = !options_.expanded;
    const double p = std::max(0.0, item.position.value());
    const double pd = std::clamp(p, 0.0, 1.0);
    const bool start_label = p >= t.label_switch_progress;
    const text::TextLine& line = start_label ? item.start_label : item.top_label;
    const double lw = line.width;
    const double lh = line.style.line_height;
    const double icon = t.icon_size;

    const double pad_h = motion::lerp((t.top_indicator_width - icon) / 2.0, t.start_indicator_leading_space, pd);
    const double pad_v = motion::lerp((t.top_indicator_height - icon) / 2.0, (t.start_indicator_height - icon) / 2.0, pd);
    const double ripple_w = motion::lerp(icon, icon + lw + t.start_icon_label_space, p) + 2.0 * pad_h;
    const double ripple_h = motion::lerp(icon, std::max(icon, lh), p) + 2.0 * pad_v;

    const double start = t.item_horizontal_padding;
    const double end = top ? t.item_horizontal_padding : 0.0;
    const double width_top = std::min(item_max_width, std::max(lw + start + end, icon + start + end + 2.0 * pad_h));
    const double width_start = std::min(item_max_width, ripple_w + start + end);
    const double content_w = motion::lerp(width_top, width_start, p);
    const double content_h = motion::lerp(ripple_h + t.top_icon_label_space + lh, ripple_h, p);

    const double icon_y_top = pad_v;
    const double icon_y_start = (content_h - icon) / 2.0 - icon_y_top;
    const double icon_y = motion::lerp(0.0, icon_y_start, p) + icon_y_top;
    const double label_x_top = (icon + 2.0 * pad_h - lw) / 2.0;
    const double label_y_top = icon_y + icon + pad_v + t.top_icon_label_space;
    const double label_offset = top && p > 0.0 ? 0.0 : start * (1.0 - p);
    const double label_x_start = pad_h + icon + t.start_icon_label_space - label_offset;
    const double label_y_start = (content_h - lh) / 2.0;
    const bool first_half = p < t.label_switch_progress;
    const double fade = (p - t.label_switch_progress) / t.label_switch_progress;

    const double width = std::clamp(content_w, t.item_min_width, std::max(t.item_min_width, item_max_width));
    const double height = std::max(content_h, item_min_height_.value());
    return {
        width,
        height,
        (width - content_w) / 2.0,
        (height - content_h) / 2.0,
        content_w,
        content_h,
        ripple_w,
        ripple_h,
        ripple_w * std::max(0.0, item.indicator.value()),
        {start + pad_h, icon_y},
        {start + (first_half ? label_x_top : label_x_start * p), first_half ? label_y_top : label_y_start},
        start_label,
        std::min(1.0, fade * fade),
    };
}

double NavigationRail::header_offset() const {
    if (!options_.header || header_size_.height <= 0.0) {
        return 0.0;
    }
    return header_size_.height + t.header_space;
}

const text::TextLine& NavigationRail::label_line(std::size_t index) const {
    return layout_[index].start_label ? items_[index].start_label : items_[index].top_label;
}

void NavigationRail::update_layout() {
    layout_.clear();
    item_tops_.clear();
    double y = t.top_space + header_offset();
    for (std::size_t i = 0; i < items_.size(); ++i) {
        layout_.push_back(geometry(i, item_max_width_));
        item_tops_.push_back(y);
        y += layout_.back().height + item_space_.value();
    }
}

void NavigationRail::update_width() {
    const double max_w = std::min(max_width_, t.expanded_max_width);
    const double actual_min = std::min(min_width_.value(), max_w);
    const double actual_expanded_min = std::min(t.expanded_min_width, max_w);
    if (options_.header) {
        header_size_ = options_.header->measure({0.0, max_w, 0.0, layout::unbounded});
    }
    double width = actual_min;
    if (options_.expanded) {
        item_max_width_ = std::max(max_w, t.item_min_width);
        double widest = options_.header ? header_size_.width : 0.0;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            widest = std::max(widest, geometry(i, item_max_width_).width);
        }
        if (widest > actual_min && widest > actual_expanded_min) {
            width = std::min(full_width_.value(), std::min(std::max(widest, actual_expanded_min), max_w));
            max_expanded_width_ = width;
        }
    } else {
        item_max_width_ = std::max(actual_min, t.item_min_width);
        if (max_expanded_width_ > 0.0) {
            width = std::clamp(full_width_.value(), actual_min, std::max(current_width_, actual_min));
        }
    }
    current_width_ = width;
    width_ = width;
    update_layout();
}

layout::Size NavigationRail::measure(const layout::Constraints& c) {
    max_width_ = c.max_width;
    update_width();
    return {width_, std::isfinite(c.max_height) ? c.max_height : height_};
}

void NavigationRail::arrange(const render::Rect& rect) {
    origin_ = {rect.x, rect.y};
    height_ = rect.height;
    if (options_.header) {
        options_.header->arrange({rect.x, rect.y + t.top_space, header_size_.width, header_size_.height});
    }
}

render::Rect NavigationRail::bounds() const {
    return {origin_.x, origin_.y, width_, height_};
}

void NavigationRail::set_expanded(bool expanded) {
    options_.expanded = expanded;
    min_width_.animate_to(expanded ? t.expanded_min_width : t.collapsed_width);
    full_width_.animate_to(expanded ? t.expanded_max_width : t.collapsed_width);
    item_space_.animate_to(expanded ? t.expanded_item_space : t.collapsed_item_space);
    item_min_height_.animate_to(expanded ? t.expanded_item_min_height : t.collapsed_item_min_height);
    for (Item& item : items_) {
        item.position.animate_to(expanded ? 1.0 : 0.0);
        item.label_mode.animate_to(expanded ? 1.0 : 0.0);
    }
    update_width();
}

void NavigationRail::select(std::size_t index) {
    select_navigation_item(items_, options_.selected, index);
}

render::Rect NavigationRail::item_rect(std::size_t index) const {
    const Geometry& g = layout_[index];
    return {origin_.x, origin_.y + item_tops_[index], g.width, g.height};
}

render::Rect NavigationRail::indicator_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry& g = layout_[index];
    return {r.x + g.content_x + t.item_horizontal_padding, r.y + g.content_y, g.indicator_width, g.ripple_height};
}

render::Rect NavigationRail::state_layer_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry& g = layout_[index];
    return {r.x + g.content_x + t.item_horizontal_padding, r.y + g.content_y, g.ripple_width, g.ripple_height};
}

render::Rect NavigationRail::icon_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry& g = layout_[index];
    return {r.x + g.content_x + g.icon.x, r.y + g.content_y + g.icon.y, t.icon_size, t.icon_size};
}

render::Rect NavigationRail::label_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry& g = layout_[index];
    const text::TextLine& line = label_line(index);
    return {r.x + g.content_x + g.label.x, r.y + g.content_y + g.label.y, line.width, line.style.line_height};
}

double NavigationRail::label_opacity(std::size_t index) const {
    return layout_[index].label_opacity;
}

bool NavigationRail::handle(const input::PointerEvent& e) {
    return select_on_click(items_, e, options_.selected, [this](std::size_t i) { return item_rect(i); },
                           [this](std::size_t i) { select(i); });
}

void NavigationRail::cancel_interaction() {
    cancel_presses(items_);
}

void NavigationRail::step(double dt) {
    min_width_.step(dt);
    full_width_.step(dt);
    item_space_.step(dt);
    item_min_height_.step(dt);
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.indicator.step(dt);
        item.position.step(dt);
        item.color.step(dt);
        item.label_mode.step(dt);
    }
    update_width();
}

bool NavigationRail::animating() const {
    if (!min_width_.settled() || !full_width_.settled() || !item_space_.settled() || !item_min_height_.settled()) {
        return true;
    }
    return motion::any_unsettled(items_, &Item::state_layer, &Item::indicator, &Item::position, &Item::color, &Item::label_mode);
}

void NavigationRail::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = bounds();
    canvas.fill_path(render::rounded_rect_path(b, render::uniform_radii(0.0)), scheme[t.container]);
    canvas.push_clip(b);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const NavigationItem& spec = options_.items[i];
        const Geometry& g = layout_[i];
        const render::Rect indicator = indicator_rect(i);
        const double progress = std::clamp(item.indicator.value(), 0.0, 1.0);
        if (progress > 0.0 && indicator.width > 0.0) {
            const double radius = std::min(indicator.width, indicator.height) / 2.0;
            canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(radius)),
                             color::with_alpha(scheme[t.indicator], progress));
        }
        if (item.state_layer.value() > 0.0) {
            const render::Rect ripple = state_layer_rect(i);
            canvas.fill_path(render::rounded_rect_path(ripple, render::uniform_radii(std::min(ripple.width, ripple.height) / 2.0)),
                             color::with_alpha(scheme[t.state_layer], item.state_layer.value()));
        }
        const double k = std::clamp(item.color.value(), 0.0, 1.0);
        const render::Rect icon = icon_rect(i);
        spec.icon.font.draw(canvas, spec.icon.symbol, {icon.x, icon.y}, t.icon_size,
                            color::mix(scheme[t.inactive_icon], scheme[t.active_icon], k), icons::mix(spec.style, spec.selected_style, k));
        if (g.label_opacity > 0.0) {
            const render::Rect label = label_rect(i);
            const double mode = std::clamp(item.label_mode.value(), 0.0, 1.0);
            const color::Argb active = color::mix(scheme[t.top_active_label], scheme[t.start_active_label], mode);
            const color::Argb c = color::mix(scheme[t.inactive_label], active, k);
            const text::TextLine& line = label_line(i);
            const double y = label.y + (label.height - line.box_height()) / 2.0;
            text::draw_line(canvas, fonts_, line, {label.x, y},
                            color::scale_alpha(c, g.label_opacity));
        }
    }
    canvas.pop_clip();
}

}
