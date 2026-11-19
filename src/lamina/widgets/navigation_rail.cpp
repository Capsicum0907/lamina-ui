#include "lamina/widgets/navigation_rail.hpp"

#include <algorithm>
#include <optional>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::NavigationRailTokens& t = tokens::navigation_rail_tokens;

constexpr tokens::SpringSpec spatial_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec color_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

double lerp(double a, double b, double k) {
    return a + (b - a) * k;
}

icons::IconStyle mix_style(const icons::IconStyle& a, const icons::IconStyle& b, double k) {
    return {lerp(a.fill, b.fill, k), lerp(a.weight, b.weight, k), lerp(a.grade, b.grade, k),
            k < 0.5 ? a.optical_size : b.optical_size};
}

double text_box(const text::TextLine& line) {
    return std::max(line.style.line_height, line.ascent + line.descent);
}

}

NavigationRail::NavigationRail(NavigationRailOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      min_width_(options_.expanded ? t.expanded_min_width : t.collapsed_width, spatial_spring),
      full_width_(options_.expanded ? t.expanded_max_width : t.collapsed_width, spatial_spring),
      item_space_(options_.expanded ? t.expanded_item_space : t.collapsed_item_space, spatial_spring),
      item_min_height_(options_.expanded ? t.expanded_item_min_height : t.collapsed_item_min_height, spatial_spring),
      max_width_(t.expanded_max_width) {
    const auto top_style = text::text_style(tokens::type_style(t.top_label));
    const auto start_style = text::text_style(tokens::type_style(t.start_label));
    const double mode = options_.expanded ? 1.0 : 0.0;
    for (std::size_t i = 0; i < options_.items.size(); ++i) {
        const double on = i == options_.selected ? 1.0 : 0.0;
        items_.push_back({text::layout_line(fonts_, options_.items[i].label, top_style),
                          text::layout_line(fonts_, options_.items[i].label, start_style), {}, motion::Animated(0.0, state_spring),
                          motion::Animated(on, spatial_spring), motion::Animated(mode, spatial_spring),
                          motion::Animated(on, color_spring), motion::Animated(mode, color_spring)});
    }
    update_width();
}

NavigationRail::Geometry NavigationRail::geometry(std::size_t index, double item_max_width) const {
    const Item& item = items_[index];
    const bool top = !options_.expanded;
    const double p = std::max(0.0, item.position.value());
    const double pd = std::clamp(p, 0.0, 1.0);
    const text::TextLine& line = p < t.label_switch_progress ? item.top_label : item.start_label;
    const double lw = line.width;
    const double lh = line.style.line_height;
    const double icon = t.icon_size;

    const double pad_h = lerp((t.top_indicator_width - icon) / 2.0, t.start_indicator_leading_space, pd);
    const double pad_v = lerp((t.top_indicator_height - icon) / 2.0, (t.start_indicator_height - icon) / 2.0, pd);
    const double ripple_w = lerp(icon, icon + lw + t.start_icon_label_space, p) + 2.0 * pad_h;
    const double ripple_h = lerp(icon, std::max(icon, lh), p) + 2.0 * pad_v;

    const double start = t.item_horizontal_padding;
    const double end = top ? t.item_horizontal_padding : 0.0;
    const double width_top = std::min(item_max_width, std::max(lw + start + end, icon + start + end + 2.0 * pad_h));
    const double width_start = std::min(item_max_width, ripple_w + start + end);
    const double content_w = lerp(width_top, width_start, p);
    const double content_h = lerp(ripple_h + t.top_icon_label_space + lh, ripple_h, p);

    const double icon_y_top = pad_v;
    const double icon_y_start = (content_h - icon) / 2.0 - icon_y_top;
    const double icon_y = lerp(0.0, icon_y_start, p) + icon_y_top;
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
        &line,
        std::min(1.0, fade * fade),
    };
}

double NavigationRail::header_offset() const {
    if (!options_.header || header_size_.height <= 0.0) {
        return 0.0;
    }
    return header_size_.height + t.header_space;
}

double NavigationRail::item_top(std::size_t index) const {
    double y = t.top_space + header_offset();
    for (std::size_t i = 0; i < index; ++i) {
        y += geometry(i, item_max_width_).height + item_space_.value();
    }
    return y;
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
}

double NavigationRail::measure_width(double max_width) {
    max_width_ = max_width;
    update_width();
    return width_;
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
    if (index >= items_.size()) {
        return;
    }
    options_.selected = index;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const double on = i == index ? 1.0 : 0.0;
        items_[i].indicator.animate_to(on);
        items_[i].color.animate_to(on);
    }
}

render::Rect NavigationRail::item_rect(std::size_t index) const {
    const Geometry g = geometry(index, item_max_width_);
    return {origin_.x, origin_.y + item_top(index), g.width, g.height};
}

render::Rect NavigationRail::indicator_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry g = geometry(index, item_max_width_);
    return {r.x + g.content_x + t.item_horizontal_padding, r.y + g.content_y, g.indicator_width, g.ripple_height};
}

render::Rect NavigationRail::state_layer_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry g = geometry(index, item_max_width_);
    return {r.x + g.content_x + t.item_horizontal_padding, r.y + g.content_y, g.ripple_width, g.ripple_height};
}

render::Rect NavigationRail::icon_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry g = geometry(index, item_max_width_);
    return {r.x + g.content_x + g.icon.x, r.y + g.content_y + g.icon.y, t.icon_size, t.icon_size};
}

render::Rect NavigationRail::label_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    const Geometry g = geometry(index, item_max_width_);
    return {r.x + g.content_x + g.label.x, r.y + g.content_y + g.label.y, g.line->width, g.line->style.line_height};
}

double NavigationRail::label_opacity(std::size_t index) const {
    return geometry(index, item_max_width_).label_opacity;
}

bool NavigationRail::handle(const input::PointerEvent& e) {
    std::optional<std::size_t> clicked;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        Item& item = items_[i];
        if (item.press.update(e, item_rect(i))) {
            clicked = i;
        }
        item.state_layer.animate_to(item.press.state_layer_opacity());
    }
    if (clicked && *clicked != options_.selected) {
        select(*clicked);
        return true;
    }
    return false;
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
    return std::any_of(items_.begin(), items_.end(), [](const Item& item) {
        return !item.state_layer.settled() || !item.indicator.settled() || !item.position.settled() || !item.color.settled() ||
               !item.label_mode.settled();
    });
}

void NavigationRail::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = bounds();
    canvas.fill_path(render::rounded_rect_path(b, render::uniform_radii(0.0)), scheme[t.container]);
    canvas.push_clip(b);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const NavigationRailItem& spec = options_.items[i];
        const Geometry g = geometry(i, item_max_width_);
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
                            color::mix(scheme[t.inactive_icon], scheme[t.active_icon], k), mix_style(spec.style, spec.selected_style, k));
        if (g.label_opacity > 0.0) {
            const render::Rect label = label_rect(i);
            const double mode = std::clamp(item.label_mode.value(), 0.0, 1.0);
            const color::Argb active = color::mix(scheme[t.top_active_label], scheme[t.start_active_label], mode);
            const color::Argb c = color::mix(scheme[t.inactive_label], active, k);
            const double y = label.y + (label.height - text_box(*g.line)) / 2.0;
            text::draw_line(canvas, fonts_, *g.line, {label.x, y},
                            color::with_alpha(c, g.label_opacity * static_cast<double>(c >> 24) / 255.0));
        }
    }
    canvas.pop_clip();
}

}
