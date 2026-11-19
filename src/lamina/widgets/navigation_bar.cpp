#include "lamina/widgets/navigation_bar.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::NavigationBarTokens& t = tokens::navigation_bar_tokens;

constexpr tokens::SpringSpec indicator_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec color_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr tokens::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

double label_height(const text::TextLine& line) {
    return line.style.line_height;
}

double lerp(double a, double b, double k) {
    return a + (b - a) * k;
}

icons::IconStyle mix_style(const icons::IconStyle& a, const icons::IconStyle& b, double k) {
    return {lerp(a.fill, b.fill, k), lerp(a.weight, b.weight, k), lerp(a.grade, b.grade, k),
            k < 0.5 ? a.optical_size : b.optical_size};
}

}

NavigationBar::NavigationBar(NavigationBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)), fonts_(fonts) {
    const auto style = text::text_style(tokens::type_style(t.label));
    for (std::size_t i = 0; i < options_.items.size(); ++i) {
        std::optional<text::TextLine> label;
        if (!options_.items[i].label.empty()) {
            label = text::layout_line(fonts_, options_.items[i].label, style);
        }
        const double on = i == options_.selected ? 1.0 : 0.0;
        items_.push_back({std::move(label), {}, motion::Animated(0.0, state_spring), motion::Animated(on, indicator_spring),
                          motion::Animated(on, color_spring)});
    }
}

double NavigationBar::height() const {
    double content = t.height;
    for (const Item& item : items_) {
        if (item.label) {
            content = std::max(content, 2.0 * t.item_vertical_padding + t.indicator_height + t.indicator_to_label_space +
                                            label_height(*item.label));
        }
    }
    return content;
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
        return r.y + (r.height - t.indicator_height) / 2.0;
    }
    const double content = 2.0 * t.item_vertical_padding + t.indicator_height + t.indicator_to_label_space + label_height(*item.label);
    return r.y + (r.height - content) / 2.0 + t.item_vertical_padding;
}

render::Rect NavigationBar::full_indicator_rect(std::size_t index) const {
    const render::Rect r = item_rect(index);
    return {r.x + (r.width - t.indicator_width) / 2.0, content_top(index), t.indicator_width, t.indicator_height};
}

render::Rect NavigationBar::indicator_rect(std::size_t index) const {
    const render::Rect full = full_indicator_rect(index);
    const double w = t.indicator_width * std::max(0.0, items_[index].indicator.value());
    return {full.x + (full.width - w) / 2.0, full.y, w, full.height};
}

void NavigationBar::select(std::size_t index) {
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

bool NavigationBar::handle(const input::PointerEvent& e) {
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

void NavigationBar::step(double dt) {
    for (Item& item : items_) {
        item.state_layer.step(dt);
        item.indicator.step(dt);
        item.color.step(dt);
    }
}

bool NavigationBar::animating() const {
    return std::any_of(items_.begin(), items_.end(), [](const Item& item) {
        return !item.state_layer.settled() || !item.indicator.settled() || !item.color.settled();
    });
}

void NavigationBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    canvas.fill_path(render::rounded_rect_path(bounds(), render::uniform_radii(0.0)), scheme[t.container]);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        const NavigationBarItem& spec = options_.items[i];
        const double progress = item.indicator.value();
        const render::Rect indicator = indicator_rect(i);
        if (progress > 0.0 && indicator.width > 0.0) {
            canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(indicator.height / 2.0)),
                             color::with_alpha(scheme[t.indicator], std::clamp(progress, 0.0, 1.0)));
        }
        const render::Rect full = full_indicator_rect(i);
        if (item.state_layer.value() > 0.0) {
            canvas.fill_path(render::rounded_rect_path(full, render::uniform_radii(full.height / 2.0)),
                             color::with_alpha(scheme[t.state_layer], item.state_layer.value()));
        }
        const double k = item.color.value();
        const render::Point icon_at{full.x + (full.width - t.icon_size) / 2.0, full.y + (full.height - t.icon_size) / 2.0};
        spec.icon.font.draw(canvas, spec.icon.symbol, icon_at, t.icon_size,
                            color::mix(scheme[t.inactive_icon], scheme[t.active_icon], k),
                            mix_style(spec.style, spec.selected_style, std::clamp(k, 0.0, 1.0)));
        if (item.label) {
            const double x = full.x + (full.width - item.label->width) / 2.0;
            text::draw_line(canvas, fonts_, *item.label, {x, full.y + full.height + t.indicator_to_label_space},
                            color::mix(scheme[t.inactive_label], scheme[t.active_label], k));
        }
    }
}

}
