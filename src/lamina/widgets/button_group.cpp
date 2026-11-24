#include "lamina/style/style.hpp"
#include "lamina/widgets/button_group.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace lamina::widgets {

namespace {

const tokens::ButtonGroupTokens& t() {
    return current_style().button_group;
}

std::optional<tokens::ConnectedPosition> connected_position(std::size_t index, std::size_t count) {
    if (count < 2) {
        return std::nullopt;
    }
    if (index == 0) {
        return tokens::ConnectedPosition::leading;
    }
    return index + 1 == count ? tokens::ConnectedPosition::trailing : tokens::ConnectedPosition::middle;
}

}

ButtonGroup::ButtonGroup(ButtonGroupOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)), fonts_(fonts), icon_font_(icon_font), enabled_(options_.enabled) {
    if (options_.items.empty()) {
        throw std::invalid_argument("A button group needs at least one item");
    }
    const std::size_t n = options_.items.size();
    bool seen = false;
    items_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        ButtonGroupItem& spec = options_.items[i];
        if (options_.selection == ButtonGroupSelection::single && spec.checked) {
            spec.checked = !seen;
            seen = true;
        }
        ButtonOptions b{spec.label, options_.style, options_.size, tokens::ButtonShape::round, spec.icon};
        b.enabled = spec.enabled && enabled_;
        b.accessible_name = spec.accessible_name;
        if (options_.selection != ButtonGroupSelection::none) {
            b.toggle = ButtonToggle{.checked = spec.checked, .radio = options_.selection == ButtonGroupSelection::single};
        }
        if (options_.connected) {
            if (const auto p = connected_position(i, n)) {
                b.shapes = ButtonShapes{current_style().button_group.rest_corners(*p), current_style().button_group.pressed_corners(*p),
                                        tokens::all_corners(t().connected_checked_corner), current_style().button.toggle_motion.shape};
            }
        }
        items_.push_back(Item{spec, std::make_unique<Button>(std::move(b), fonts_), motion::Animated(0.0, t().width_spring)});
    }
    IconButtonOptions overflow{icons::Icon{icon_font_, t().overflow_symbol}, options_.strings->more_options, t().overflow_style,
                               static_cast<tokens::IconButtonSize>(static_cast<std::size_t>(options_.size))};
    overflow.enabled = enabled_;
    overflow_ = std::make_unique<IconButton>(std::move(overflow));
    visible_ = n;
}

double ButtonGroup::spacing() const {
    return options_.connected ? t().connected_between_space : t().between_space;
}

double ButtonGroup::compression_limit(std::size_t) const {
    return current_style().button.size(options_.size).trailing_space;
}

layout::Size ButtonGroup::measure(const layout::Constraints& c) {
    const std::size_t n = items_.size();
    const double gap = spacing();
    max_width_ = c.max_width;
    double fixed = 0.0;
    double total_weight = 0.0;
    for (Item& item : items_) {
        if (item.spec.weight > 0.0) {
            total_weight += item.spec.weight;
        } else {
            item.base = item.button->intrinsic_width();
            fixed += item.base;
        }
    }
    if (total_weight > 0.0) {
        const double target = std::isfinite(c.max_width) ? c.max_width : c.min_width;
        const double room = std::max(0.0, target - fixed - gap * static_cast<double>(n - 1));
        for (Item& item : items_) {
            if (item.spec.weight > 0.0) {
                item.base = room * item.spec.weight / total_weight;
            }
        }
    }
    double desired = gap * static_cast<double>(n - 1);
    for (const Item& item : items_) {
        desired += item.base;
    }
    double main = desired;
    std::size_t last = n;
    if (std::isfinite(c.max_width) && desired > c.max_width) {
        const double overflow_width = overflow_->width();
        double remaining = c.max_width - overflow_width;
        main = overflow_width;
        last = 0;
        while (last < n && items_[last].base <= remaining) {
            main += items_[last].base;
            remaining -= items_[last].base + gap;
            ++last;
        }
        main += gap * static_cast<double>(last);
    }
    if (last != visible_) {
        visible_ = last;
        menu_.reset();
        for (std::size_t i = visible_; i < n; ++i) {
            items_[i].button->cancel_interaction();
            items_[i].button->set_focused(false);
        }
        if (!overflowing()) {
            overflow_->set_focused(false);
        }
    }
    double height = 0.0;
    for (const Item& item : items_) {
        height = std::max(height, item.button->height());
    }
    if (overflowing()) {
        height = std::max(height, overflow_->height());
    }
    return {std::max(main, c.min_width), height};
}

void ButtonGroup::arrange(const render::Rect& rect) {
    bounds_ = rect;
    layout_items();
}

void ButtonGroup::layout_items() {
    const std::size_t n = items_.size();
    for (Item& item : items_) {
        item.width = item.base;
    }
    if (n > 1 && options_.animate_width) {
        const double ratio = options_.expanded_ratio;
        for (std::size_t i = 0; i < visible_; ++i) {
            const double a = items_[i].pressed.value();
            if (a == 0.0) {
                continue;
            }
            double growth = 0.0;
            if (i >= 1 && i + 1 < visible_) {
                const double target = a * std::min({ratio * items_[i].width / 2.0, compression_limit(i - 1), compression_limit(i + 1)});
                const double left = std::min(target, items_[i - 1].width);
                const double right = std::min(target, items_[i + 1].width);
                items_[i - 1].width -= left;
                items_[i + 1].width -= right;
                growth = left + right;
            } else if (i == 0) {
                if (i + 1 >= n) {
                    continue;
                }
                const double target = a * std::min(ratio * items_[i].width, compression_limit(i + 1));
                const double right = std::min(target, items_[i + 1].width);
                items_[i + 1].width -= right;
                growth = right;
            } else {
                const double target = a * std::min(ratio * items_[i].width, compression_limit(i - 1));
                const double left = std::min(target, items_[i - 1].width);
                items_[i - 1].width -= left;
                growth = left;
            }
            items_[i].width += growth;
        }
    }
    double x = bounds_.x;
    for (std::size_t i = 0; i < visible_; ++i) {
        Button& b = *items_[i].button;
        b.set_size(items_[i].width, b.height());
        b.set_origin({x, bounds_.y + (bounds_.height - b.height()) / 2.0});
        x += items_[i].width + spacing();
    }
    if (overflowing()) {
        double shown = 0.0;
        for (std::size_t i = 0; i < visible_; ++i) {
            shown += items_[i].base + spacing();
        }
        overflow_->set_origin({bounds_.x + shown, bounds_.y + (bounds_.height - overflow_->height()) / 2.0});
    }
}

std::vector<FocusStop*> ButtonGroup::focus_stops() {
    if (stops_.empty()) {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            stops_.push_back(std::make_unique<FocusStop>(forward_focus(*items_[i].button, [this, i] { return i < visible_; })));
        }
        stops_.push_back(std::make_unique<FocusStop>(forward_focus(*overflow_, [this] { return overflowing(); })));
    }
    std::vector<FocusStop*> out;
    for (auto& s : stops_) {
        out.push_back(s.get());
    }
    return out;
}

void ButtonGroup::set_checked(std::size_t index, bool checked) {
    if (options_.selection == ButtonGroupSelection::none) {
        return;
    }
    if (options_.selection == ButtonGroupSelection::single && checked) {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (i != index) {
                items_[i].button->set_checked(false);
            }
        }
    }
    items_[index].button->set_checked(checked);
}

std::optional<std::size_t> ButtonGroup::selected() const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].button->checked()) {
            return i;
        }
    }
    return std::nullopt;
}

void ButtonGroup::set_enabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    for (Item& item : items_) {
        item.button->set_enabled(enabled && item.spec.enabled);
        if (!enabled) {
            item.pressed.animate_to(0.0);
            item.releasing = false;
            item.was_pressed = false;
        }
    }
    overflow_->set_enabled(enabled);
    if (!enabled && menu_) {
        menu_->close();
    }
}

AccessibleNode ButtonGroup::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name, bounds_, {.enabled = enabled_}};
    for (std::size_t i = 0; i < visible_; ++i) {
        node.children.push_back(items_[i].button->accessible());
    }
    if (overflowing()) {
        node.children.push_back(overflow_->accessible());
    }
    if (menu_) {
        if (auto m = menu_->accessible()) {
            node.children.push_back(std::move(*m));
        }
    }
    return node;
}

void ButtonGroup::track_press(Item& item) {
    const bool now = item.button->pressed();
    if (now == item.was_pressed) {
        return;
    }
    item.was_pressed = now;
    if (now) {
        item.releasing = false;
        item.pressed.animate_to(1.0);
    } else {
        item.releasing = true;
    }
}

void ButtonGroup::choose(std::size_t index, ButtonGroupResponse& response) {
    response.clicked = index;
    response.consumed = true;
    if (options_.selection == ButtonGroupSelection::single) {
        set_checked(index, true);
    } else if (options_.selection == ButtonGroupSelection::multiple) {
        set_checked(index, !checked(index));
    }
}

void ButtonGroup::open_menu(input::InputSource source) {
    if (!menu_) {
        MenuOptions m;
        m.accessible_name = options_.strings->more_options;
        menu_items_.clear();
        for (std::size_t i = visible_; i < items_.size(); ++i) {
            m.items.push_back({items_[i].spec.label, items_[i].spec.icon, std::nullopt, items_[i].spec.enabled});
            menu_items_.push_back(i);
        }
        menu_ = std::make_unique<Menu>(std::move(m), fonts_);
    }
    menu_->open(overflow_->bounds(), window_, source);
}

template <typename Offer>
void ButtonGroup::offer_targets(const input::PointerEvent& e, Offer offer) const {
    for (std::size_t i = 0; i < visible_; ++i) {
        offer(items_[i].button.get(), items_[i].button->target_hit(e));
    }
    if (overflowing()) {
        offer(overflow_.get(), overflow_->target_hit(e));
    }
}

std::optional<TargetHit> ButtonGroup::target_hit(const input::PointerEvent& e) const {
    std::optional<TargetHit> best;
    offer_targets(e, [&](const void*, const std::optional<TargetHit>& h) { best = best_hit(best, h); });
    return best;
}

ButtonGroupResponse ButtonGroup::handle(const input::PointerEvent& event) {
    arbiter_.resolve(event, [&](auto offer) { offer_targets(event, offer); });
    const input::PointerEvent e = arbiter_.routed(event, overflow_.get());
    ButtonGroupResponse response;
    const bool inside_overflow = overflowing() && overflow_->hits(e);
    if (e.action == input::PointerAction::down && !inside_overflow) {
        reopen_blocked_ = false;
    }
    if (menu_open()) {
        const MenuResponse m = menu_->handle(event);
        if (m.chosen) {
            choose(menu_items_[*m.chosen], response);
            menu_->close();
            return response;
        }
        if (m.consumed && !inside_overflow) {
            response.consumed = true;
            return response;
        }
        if (inside_overflow && !menu_->is_open()) {
            reopen_blocked_ = true;
        }
    }
    for (std::size_t i = 0; i < visible_; ++i) {
        Item& item = items_[i];
        const bool clicked = item.button->handle(arbiter_.routed(event, item.button.get()));
        track_press(item);
        if (clicked) {
            response.clicked = i;
            response.consumed = true;
            if (options_.selection == ButtonGroupSelection::single) {
                set_checked(i, true);
            }
        }
    }
    if (overflowing() && overflow_->handle(e)) {
        response.consumed = true;
        if (reopen_blocked_) {
            reopen_blocked_ = false;
        } else {
            open_menu(input::InputSource::pointer);
        }
    }
    return response;
}

ButtonGroupResponse ButtonGroup::handle(const input::KeyEvent& e) {
    ButtonGroupResponse response;
    if (menu_open()) {
        const MenuResponse m = menu_->handle(e);
        if (m.chosen) {
            choose(menu_items_[*m.chosen], response);
            menu_->close();
            overflow_->set_focused(true);
            return response;
        }
        if (!menu_->is_open()) {
            overflow_->set_focused(true);
        }
        response.consumed = m.consumed || response.consumed;
        return response;
    }
    for (std::size_t i = 0; i < visible_; ++i) {
        if (items_[i].button->handle(e)) {
            response.clicked = i;
            response.consumed = true;
            if (options_.selection == ButtonGroupSelection::single) {
                set_checked(i, true);
            }
        }
    }
    if (overflowing() && overflow_->handle(e)) {
        response.consumed = true;
        open_menu(input::InputSource::keyboard);
    }
    return response;
}

void ButtonGroup::cancel_interaction() {
    for (Item& item : items_) {
        item.button->cancel_interaction();
        track_press(item);
    }
    overflow_->cancel_interaction();
    if (menu_) {
        menu_->cancel_interaction();
    }
}

void ButtonGroup::step(double dt) {
    for (Item& item : items_) {
        item.button->step(dt);
        item.pressed.step(dt);
        if (item.releasing && item.pressed.value() > t().release_threshold) {
            item.releasing = false;
            item.pressed.animate_to(0.0);
        }
    }
    overflow_->step(dt);
    if (menu_) {
        menu_->step(dt);
    }
    layout_items();
}

bool ButtonGroup::animating() const {
    const bool items = std::any_of(items_.begin(), items_.end(), [](const Item& item) {
        return item.button->animating() || !item.pressed.settled() || item.releasing;
    });
    return items || overflow_->animating() || (menu_ && menu_->animating());
}

void ButtonGroup::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    for (std::size_t i = 0; i < visible_; ++i) {
        items_[i].button->paint(canvas, scheme);
    }
    if (overflowing()) {
        overflow_->paint(canvas, scheme);
    }
}

void ButtonGroup::paint_overlay(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (menu_ && menu_->visible()) {
        menu_->paint(canvas, scheme);
    }
}

}
