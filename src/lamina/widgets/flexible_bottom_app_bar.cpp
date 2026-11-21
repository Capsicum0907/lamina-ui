#include "lamina/widgets/flexible_bottom_app_bar.hpp"

#include <algorithm>

namespace lamina::widgets {

namespace {

constexpr const tokens::FlexibleBottomAppBarTokens& t = tokens::flexible_bottom_app_bar_tokens;

}

BottomAppBarState::BottomAppBarState(bool exit_always) : exit_always_(exit_always), settle_(0.0, t.snap) {}

void BottomAppBarState::set_height_offset_limit(double limit) {
    limit_ = std::min(0.0, limit);
    height_offset_ = std::clamp(height_offset_, limit_, 0.0);
}

void BottomAppBarState::scrolled_to(double offset) {
    const double delta = offset - last_;
    last_ = offset;
    if (!exit_always_ || delta == 0.0) {
        return;
    }
    settling_ = false;
    height_offset_ = std::clamp(height_offset_ - delta, limit_, 0.0);
}

bool BottomAppBarState::needs_settle() const {
    if (!exit_always_ || settling_) {
        return false;
    }
    const double f = collapsed_fraction();
    return f >= t.settle_threshold && f < 1.0 && height_offset_ < 0.0 && height_offset_ > limit_;
}

void BottomAppBarState::settle() {
    if (!needs_settle()) {
        return;
    }
    settling_ = true;
    settle_.snap_to(height_offset_);
    settle_.animate_to(collapsed_fraction() < t.settle_midpoint ? 0.0 : limit_);
}

void BottomAppBarState::step(double dt) {
    if (!settling_) {
        return;
    }
    settle_.step(dt);
    height_offset_ = std::clamp(settle_.value(), limit_, 0.0);
    if (settle_.settled()) {
        height_offset_ = settle_.target();
        settling_ = false;
    }
}

FlexibleBottomAppBar::FlexibleBottomAppBar(FlexibleBottomAppBarOptions options)
    : options_(std::move(options)), state_(options_.exit_always) {
    for (const ToolbarAction& a : options_.actions) {
        IconButtonOptions o{a.icon, a.accessible_name, tokens::IconButtonStyle::standard};
        o.enabled = a.enabled;
        o.content = t.content;
        actions_.push_back(std::make_unique<IconButton>(std::move(o)));
    }
    state_.set_height_offset_limit(-t.height);
}

double FlexibleBottomAppBar::height() const {
    return std::max(0.0, t.height + state_.height_offset());
}

void FlexibleBottomAppBar::set_width(double width) {
    width_ = width;
    place();
}

void FlexibleBottomAppBar::set_origin(render::Point origin) {
    origin_ = origin;
    bottom_.reset();
    place();
}

void FlexibleBottomAppBar::set_bottom(double bottom) {
    bottom_ = bottom;
    place();
}

void FlexibleBottomAppBar::follow(const ScrollArea& scroll) {
    state_.scrolled_to(scroll.offset());
    if (scroll.target_offset() == scroll.offset() && !scroll.dragging()) {
        state_.settle();
    }
    place();
}

void FlexibleBottomAppBar::place() {
    if (bottom_) {
        origin_.y = *bottom_ - height();
    }
    const double slot = t.item_slot;
    const double n = static_cast<double>(actions_.size());
    const double inner = width_ - t.leading_space - t.trailing_space;
    double x = origin_.x + t.leading_space;
    double gap = 0.0;
    if (options_.arrangement == BottomBarArrangement::space_between) {
        gap = n > 1.0 ? std::max(0.0, (inner - n * slot) / (n - 1.0)) : 0.0;
        if (n == 1.0) {
            x += (inner - slot) / 2.0;
        }
    } else {
        gap = t.fixed_spacing;
        x += std::max(0.0, (inner - n * slot - (n - 1.0) * gap) / 2.0);
    }
    const double y = origin_.y + (t.height - slot) / 2.0;
    for (auto& b : actions_) {
        b->set_origin({x + (slot - b->width()) / 2.0, y + (slot - b->height()) / 2.0});
        x += slot + gap;
    }
}

std::vector<FocusStop*> FlexibleBottomAppBar::focus_stops() {
    if (stops_.empty()) {
        for (auto& b : actions_) {
            stops_.push_back(std::make_unique<FocusStop>(forward_focus(*b, [this] { return shown(); })));
        }
    }
    std::vector<FocusStop*> out;
    for (auto& s : stops_) {
        out.push_back(s.get());
    }
    return out;
}

void FlexibleBottomAppBar::set_enabled(bool enabled) {
    enabled_ = enabled;
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        actions_[i]->set_enabled(enabled && options_.actions[i].enabled);
    }
}

AccessibleNode FlexibleBottomAppBar::accessible() const {
    AccessibleNode node{AccessibleRole::toolbar, options_.accessible_name, bounds(), {.enabled = enabled_}};
    if (shown()) {
        for (const auto& b : actions_) {
            node.children.push_back(b->accessible());
        }
    }
    return node;
}

std::optional<std::size_t> FlexibleBottomAppBar::handle(const input::PointerEvent& e) {
    std::optional<std::size_t> clicked;
    if (!shown()) {
        return clicked;
    }
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i]->handle(e)) {
            clicked = i;
        }
    }
    return clicked;
}

std::optional<std::size_t> FlexibleBottomAppBar::handle(const input::KeyEvent& e) {
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i]->handle(e)) {
            return i;
        }
    }
    return std::nullopt;
}

void FlexibleBottomAppBar::cancel_interaction() {
    for (auto& b : actions_) {
        b->cancel_interaction();
    }
}

void FlexibleBottomAppBar::step(double dt) {
    state_.step(dt);
    for (auto& b : actions_) {
        b->step(dt);
    }
    place();
}

bool FlexibleBottomAppBar::animating() const {
    return state_.settling() || std::any_of(actions_.begin(), actions_.end(), [](const auto& b) { return b->animating(); });
}

void FlexibleBottomAppBar::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect r = bounds();
    if (r.height <= 0.0) {
        return;
    }
    canvas.save();
    canvas.push_clip(r);
    canvas.fill_rect(r, scheme[t.container]);
    for (const auto& b : actions_) {
        b->paint(canvas, scheme);
    }
    canvas.pop_clip();
    canvas.restore();
}

}
