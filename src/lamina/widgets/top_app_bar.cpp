#include "lamina/widgets/top_app_bar.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/easing.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::TopAppBarTokens& t = tokens::top_app_bar_tokens;

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

bool takes_subtitle(tokens::TopAppBarKind kind) {
    return kind != tokens::TopAppBarKind::medium && kind != tokens::TopAppBarKind::large;
}

IconButtonOptions with_content(IconButtonOptions o, color::Role role) {
    if (!o.content) {
        o.content = role;
    }
    return o;
}

}

TopAppBar::TopAppBar(TopAppBarOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      fonts_(fonts),
      title_(text::layout_line(fonts_, options_.title, style_of(kind().title))),
      small_title_(text::layout_line(fonts_, options_.title, style_of(t.collapsed_title))),
      state_(options_.scroll),
      container_(0.0, t.container_spring) {
    if (!takes_subtitle(options_.kind)) {
        options_.subtitle.clear();
    }
    if (options_.navigation) {
        navigation_.emplace(with_content(*options_.navigation, t.navigation_icon));
    }
    for (const IconButtonOptions& a : options_.actions) {
        actions_.emplace_back(with_content(a, t.action_icon));
    }
    layout_titles();
    state_.set_height_offset_limit(two_rows() ? collapsed_height() - expanded_height() : -collapsed_height());
}

void TopAppBar::layout_titles() {
    title_ = text::layout_line(fonts_, options_.title, style_of(kind().title));
    small_title_ = text::layout_line(fonts_, options_.title, style_of(t.collapsed_title));
    subtitle_.reset();
    small_subtitle_.reset();
    if (has_subtitle()) {
        subtitle_ = text::layout_line(fonts_, options_.subtitle, style_of(kind().subtitle));
        small_subtitle_ = text::layout_line(fonts_, options_.subtitle, style_of(t.collapsed_subtitle));
    }
}

void TopAppBar::set_title(std::string title) {
    options_.title = std::move(title);
    layout_titles();
}

TitleAlignment TopAppBar::alignment() const {
    if (options_.alignment) {
        return *options_.alignment;
    }
    return options_.kind == tokens::TopAppBarKind::center_aligned ? TitleAlignment::center : TitleAlignment::start;
}

double TopAppBar::expanded_height() const {
    const double removed = t.collapsed_height - collapsed_height();
    return (has_subtitle() ? kind().expanded_height_with_subtitle : kind().expanded_height) - removed;
}

double TopAppBar::collapsed_height() const {
    return tokens::density_tokens.top_app_bar.at(options_.density);
}

void TopAppBar::set_density(tokens::Density density) {
    options_.density = density;
    state_.set_height_offset_limit(two_rows() ? collapsed_height() - expanded_height() : -collapsed_height());
    place_buttons();
}

double TopAppBar::height() const {
    return std::max(0.0, expanded_height() + state_.height_offset());
}

layout::Size TopAppBar::measure(const layout::Constraints& c) const {
    return c.constrain({std::isfinite(c.max_width) ? c.max_width : c.min_width, height()});
}

void TopAppBar::arrange(const render::Rect& r) {
    origin_ = {r.x, r.y};
    width_ = r.width;
    place_buttons();
}

double TopAppBar::title_start() const {
    const double nav = navigation_ ? t.horizontal_padding + t.icon_slot : 0.0;
    return std::max(t.title_inset, nav);
}

double TopAppBar::actions_width() const {
    return actions_.empty() ? 0.0 : static_cast<double>(actions_.size()) * t.icon_slot + t.horizontal_padding;
}

double TopAppBar::title_x(double box_width, double start, double end) const {
    double x = alignment() == TitleAlignment::center ? (width_ - box_width) / 2.0 : 0.0;
    if (x < start) {
        x = start;
    } else if (x + box_width > width_ - end) {
        x = width_ - end - box_width;
    }
    return origin_.x + x;
}

double TopAppBar::block_height(const text::TextLine& title, const std::optional<text::TextLine>& subtitle) const {
    return title.box_height() + (subtitle ? subtitle->box_height() : 0.0);
}

void TopAppBar::place_buttons() {
    const double row = two_rows() ? collapsed_height() : height();
    const double cy = origin_.y + row / 2.0;
    if (navigation_) {
        const double slot = origin_.x + t.horizontal_padding;
        navigation_->set_origin({slot + (t.icon_slot - navigation_->width()) / 2.0, cy - navigation_->height() / 2.0});
    }
    double x = origin_.x + width_ - t.horizontal_padding - static_cast<double>(actions_.size()) * t.icon_slot;
    for (IconButton& a : actions_) {
        a.set_origin({x + (t.icon_slot - a.width()) / 2.0, cy - a.height() / 2.0});
        x += t.icon_slot;
    }
}

double TopAppBar::title_opacity() const {
    return two_rows() ? 1.0 - state_.collapsed_fraction() : 1.0;
}

double TopAppBar::collapsed_title_opacity() const {
    return two_rows() ? motion::ease(t.top_title_alpha, state_.collapsed_fraction()) : 0.0;
}

double TopAppBar::container_amount() const {
    if (two_rows()) {
        return motion::ease(t.container_easing, state_.collapsed_fraction());
    }
    return std::clamp(container_.value(), 0.0, 1.0);
}

render::Rect TopAppBar::title_rect() const {
    const double pad = t.horizontal_padding;
    const double start = title_start();
    const double end = actions_width();
    if (!two_rows()) {
        const double w = std::max(title_.width, subtitle_ ? subtitle_->width : 0.0);
        const double h = block_height(title_, subtitle_);
        return {title_x(w + 2.0 * pad, start, end) + pad, origin_.y + (height() - h) / 2.0, w, h};
    }
    const double w = std::max(title_.width, subtitle_ ? subtitle_->width : 0.0);
    const double h = block_height(title_, subtitle_);
    const double last_baseline = subtitle_ ? title_.box_height() + subtitle_->baseline : title_.baseline;
    const double row_max = expanded_height() - collapsed_height();
    double from_bottom = kind().title_bottom_padding - (h - last_baseline);
    if (from_bottom + h > row_max) {
        from_bottom -= from_bottom + h - row_max;
    }
    const double bottom = origin_.y + height();
    return {title_x(w + 2.0 * pad, t.title_inset, 0.0) + pad, bottom - h - std::max(0.0, from_bottom), w, h};
}

void TopAppBar::follow(ScrollArea& scroll) {
    if (state_.settling()) {
        if (scroll.offset() != driven_offset_ || scroll.target_offset() != scroll.offset() || scroll.scroll_in_progress()) {
            state_.cancel_settle();
        } else {
            scroll.jump_to(state_.scroll_offset());
            driven_offset_ = scroll.offset();
            place_buttons();
            return;
        }
    }
    state_.scrolled_to(scroll.offset());
    if (scroll.target_offset() == scroll.offset() && !scroll.scroll_in_progress()) {
        state_.settle();
        driven_offset_ = scroll.offset();
    }
    container_.animate_to(state_.overlapped_fraction() > t.overlap_threshold ? 1.0 : 0.0);
    place_buttons();
}

void TopAppBar::set_enabled(bool enabled) {
    enabled_ = enabled;
    if (navigation_) {
        navigation_->set_enabled(enabled);
    }
    for (IconButton& a : actions_) {
        a.set_enabled(enabled);
    }
}

AccessibleNode TopAppBar::accessible() const {
    AccessibleNode node{AccessibleRole::toolbar, options_.accessible_name.empty() ? options_.title : options_.accessible_name, bounds(),
                        {.enabled = enabled_}};
    if (navigation_) {
        node.children.push_back(navigation_->accessible());
    }
    AccessibleNode title{AccessibleRole::label, options_.title, title_rect()};
    title.description = options_.subtitle;
    if (two_rows() && state_.collapsed_fraction() >= t.settle_midpoint) {
        const double cy = origin_.y + collapsed_height() / 2.0;
        title.bounds = {title_x(small_title_.width + 2.0 * t.horizontal_padding, title_start(), actions_width()) + t.horizontal_padding,
                        cy - small_title_.box_height() / 2.0, small_title_.width, small_title_.box_height()};
    }
    node.children.push_back(std::move(title));
    for (const IconButton& a : actions_) {
        node.children.push_back(a.accessible());
    }
    return node;
}

TopAppBarResponse TopAppBar::handle(const input::PointerEvent& e) {
    TopAppBarResponse response;
    const bool inside = e.action != input::PointerAction::leave && contains(bounds(), e.x_dp, e.y_dp);
    const input::PointerEvent routed = inside || e.action == input::PointerAction::up ? e : input::PointerEvent{input::PointerAction::leave, 0.0, 0.0};
    if (navigation_ && navigation_->handle(routed)) {
        response.navigation = true;
    }
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i].handle(routed)) {
            response.action = i;
        }
    }
    response.consumed = inside || response.navigation || response.action.has_value();
    return response;
}

TopAppBarResponse TopAppBar::handle(const input::KeyEvent& e) {
    TopAppBarResponse response;
    if (navigation_ && navigation_->handle(e)) {
        response.navigation = true;
    }
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i].handle(e)) {
            response.action = i;
        }
    }
    response.consumed = response.navigation || response.action.has_value();
    return response;
}

void TopAppBar::cancel_interaction() {
    if (navigation_) {
        navigation_->cancel_interaction();
    }
    for (IconButton& a : actions_) {
        a.cancel_interaction();
    }
}

void TopAppBar::step(double dt) {
    state_.step(dt);
    container_.step(dt);
    if (navigation_) {
        navigation_->step(dt);
    }
    for (IconButton& a : actions_) {
        a.step(dt);
    }
    place_buttons();
}

bool TopAppBar::animating() const {
    bool buttons = navigation_ && navigation_->animating();
    for (const IconButton& a : actions_) {
        buttons = buttons || a.animating();
    }
    return buttons || state_.settling() || !container_.settled();
}

void TopAppBar::paint_title(render::Canvas& canvas, const text::TextLine& title, const std::optional<text::TextLine>& subtitle, render::Point at,
                            const color::ColorScheme& scheme, double opacity) const {
    if (opacity <= 0.0) {
        return;
    }
    text::draw_line(canvas, fonts_, title, at, color::scale_alpha(scheme[t.title], opacity));
    if (subtitle) {
        text::draw_line(canvas, fonts_, *subtitle, {at.x, at.y + title.box_height()}, color::scale_alpha(scheme[t.subtitle], opacity));
    }
}

void TopAppBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    if (r.height <= 0.0 || r.width <= 0.0) {
        return;
    }
    canvas.push_clip(r);
    canvas.fill_rect(r, color::mix(scheme[t.container], scheme[t.scrolled_container], container_amount()));
    const double pad = t.horizontal_padding;
    const double start = title_start();
    const double end = actions_width();
    const double text_room = std::max(0.0, width_ - start - end - 2.0 * pad);
    if (two_rows()) {
        const double top = collapsed_title_opacity();
        if (top > 0.0) {
            const double w = std::min(text_room, std::max(small_title_.width, small_subtitle_ ? small_subtitle_->width : 0.0));
            const double h = block_height(small_title_, small_subtitle_);
            const double x = title_x(w + 2.0 * pad, start, end) + pad;
            canvas.push_clip(render::Rect{x, r.y, text_room, collapsed_height()});
            paint_title(canvas, small_title_, small_subtitle_, {x, r.y + (collapsed_height() - h) / 2.0}, scheme, top);
            canvas.pop_clip();
        }
        const render::Rect row{r.x, r.y + collapsed_height(), r.width, std::max(0.0, r.height - collapsed_height())};
        if (row.height > 0.0) {
            canvas.push_clip(row);
            const render::Rect title = title_rect();
            paint_title(canvas, title_, subtitle_, {title.x, title.y}, scheme, title_opacity());
            canvas.pop_clip();
        }
    } else {
        const render::Rect title = title_rect();
        canvas.push_clip(render::Rect{title.x, r.y, text_room, r.height});
        paint_title(canvas, title_, subtitle_, {title.x, title.y}, scheme, 1.0);
        canvas.pop_clip();
    }
    if (navigation_) {
        navigation_->paint(canvas, scheme);
    }
    for (const IconButton& a : actions_) {
        a.paint(canvas, scheme);
    }
    canvas.pop_clip();
}

}
