#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/tabs.hpp"

#include <algorithm>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::TabTokens& t = tokens::tab_tokens;

constexpr motion::SpringSpec indicator_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::spatial, tokens::SpringSpeed::standard);
constexpr motion::SpringSpec fade_in_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::standard);
constexpr motion::SpringSpec fade_out_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);
constexpr motion::SpringSpec state_spring =
    tokens::expressive_motion.spring(tokens::SpringKind::effects, tokens::SpringSpeed::fast);

}

TabRow::TabRow(TabRowOptions options, const text::FontStack& fonts)
    : options_(std::move(options)), fonts_(fonts), indicator_offset_(0.0, indicator_spring), indicator_width_(0.0, indicator_spring) {
    const auto style = tokens::text_style(tokens::type_style(t.label));
    bool stacked = false;
    for (std::size_t i = 0; i < options_.tabs.size(); ++i) {
        const TabItem& item = options_.tabs[i];
        std::optional<text::TextLine> label;
        if (!item.label.empty()) {
            label = text::layout_line(fonts_, item.label, style);
        }
        stacked = stacked || (label && item.icon && options_.icon_position == TabIconPosition::top);
        const double on = i == options_.selected ? 1.0 : 0.0;
        tabs_.push_back({std::move(label), item.icon, {}, motion::Animated(0.0, state_spring), motion::Animated(on, fade_in_spring)});
    }
    height_ = stacked ? t.icon_and_label_height : t.height;
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

bool TabRow::handle(const input::PointerEvent& e) {
    return select_on_click(tabs_, e, options_.selected, [this](std::size_t i) { return tab_rect(i); },
                           [this](std::size_t i) { select(i); });
}

void TabRow::step(double dt) {
    indicator_offset_.step(dt);
    indicator_width_.step(dt);
    for (Tab& tab : tabs_) {
        tab.state_layer.step(dt);
        tab.selected.step(dt);
    }
}

bool TabRow::animating() const {
    if (!indicator_offset_.settled() || !indicator_width_.settled()) {
        return true;
    }
    return motion::any_unsettled(tabs_, &Tab::state_layer, &Tab::selected);
}

void TabRow::paint_tab(render::Canvas& canvas, const color::ColorScheme& scheme, const Tab& tab, const render::Rect& r) const {
    const auto& style = tokens::tab_style(options_.style);
    const color::Argb active = scheme[style.active_content];
    if (tab.state_layer.value() > 0.0) {
        canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(0.0)), color::with_alpha(active, tab.state_layer.value()));
    }
    const color::Argb content = color::mix(scheme[style.inactive_content], active, tab.selected.value());
    const double cx = r.x + r.width / 2.0;
    const double cy = r.y + r.height / 2.0;

    if (tab.label && tab.icon && options_.icon_position == TabIconPosition::top) {
        const double baseline = r.y + r.height - t.text_baseline_with_icon - style.indicator_height;
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
    canvas.fill_path(render::rounded_rect_path(r, render::uniform_radii(0.0)), scheme[style.container]);
    canvas.push_clip(r);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        paint_tab(canvas, scheme, tabs_[i], tab_rect(i));
    }
    canvas.pop_clip();
    const render::Rect divider{r.x, r.y + r.height - t.divider_thickness, r.width, t.divider_thickness};
    canvas.fill_path(render::rounded_rect_path(divider, render::uniform_radii(0.0)), scheme[t.divider]);
    const render::Rect indicator = indicator_rect();
    canvas.fill_path(render::rounded_rect_path(indicator, render::uniform_radii(style.indicator_corner)), scheme[style.indicator]);
}

}
