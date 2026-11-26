#include "lamina/style/style.hpp"
#include "lamina/widgets/navigation_suite.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/tokens/navigation_suite.hpp"

namespace lamina::widgets {

namespace {

const tokens::NavigationSuiteTokens& t() {
    return current_style().navigation_suite;
}

bool side_type(NavigationSuiteType type) {
    return type == NavigationSuiteType::wide_navigation_rail_collapsed || type == NavigationSuiteType::wide_navigation_rail_expanded ||
           type == NavigationSuiteType::navigation_drawer;
}

}

NavigationSuiteType navigation_suite_type(const WindowAdaptiveInfo& info) {
    if (info.window_size_class.width_class() == layout::WindowWidthClass::compact) {
        return NavigationSuiteType::short_navigation_bar_compact;
    }
    if (info.tabletop || info.window_size_class.height_class() == layout::WindowHeightClass::compact) {
        return NavigationSuiteType::short_navigation_bar_medium;
    }
    return NavigationSuiteType::wide_navigation_rail_collapsed;
}

NavigationSuiteType navigation_suite_type(const WindowAdaptiveInfo& info, const NavigationSuiteChoice& choice) {
    const layout::WindowWidthClass w = info.window_size_class.width_class();
    if (w != layout::WindowWidthClass::compact &&
        (info.tabletop || info.window_size_class.height_class() == layout::WindowHeightClass::compact)) {
        return choice.compact_height;
    }
    return choice.by_width[static_cast<std::size_t>(w)];
}

NavigationSuiteScaffold::NavigationSuiteScaffold(NavigationSuiteScaffoldOptions options, const text::FontStack& fonts)
    : type_(options.type),
      selected_(options.selected),
      content_node_(options.content),
      bar_widget_(NavigationBarOptions{options.items, options.selected, options.accessible_name, options.density, options.labels}, fonts),
      side_widget_(SideNavigationOptions{options.items, options.selected, options.labels && is_expanded(options.type), is_drawer(options.type),
                                         options.accessible_name.empty() ? options.strings->navigation_menu : options.accessible_name,
                                         options.density},
                   fonts),
      bar_(is_bar(options.type) ? 1.0 : 0.0, t().spring),
      side_(side_type(options.type) ? 1.0 : 0.0, t().spring),
      labels_(options.labels),
      density_(options.density) {}

bool NavigationSuiteScaffold::is_bar(NavigationSuiteType type) {
    return type == NavigationSuiteType::short_navigation_bar_compact || type == NavigationSuiteType::short_navigation_bar_medium;
}

bool NavigationSuiteScaffold::is_drawer(NavigationSuiteType type) {
    return type == NavigationSuiteType::navigation_drawer;
}

bool NavigationSuiteScaffold::is_expanded(NavigationSuiteType type) {
    return type == NavigationSuiteType::wide_navigation_rail_expanded || type == NavigationSuiteType::navigation_drawer;
}

NavigationSuiteScaffold::Active NavigationSuiteScaffold::active() const {
    if (is_bar(type_)) {
        return Active::bar;
    }
    return side_type(type_) ? Active::side : Active::none;
}

bool NavigationSuiteScaffold::bar_visible() const {
    return bar_.value() > 0.0;
}

bool NavigationSuiteScaffold::side_visible() const {
    return side_.value() > 0.0;
}

layout::Size NavigationSuiteScaffold::measure(const layout::Constraints& c) const {
    const double w = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : c.min_height;
    return c.constrain({w, h});
}

double NavigationSuiteScaffold::side_width() const {
    return std::max(0.0, side_.value()) * side_widget_.width();
}

void NavigationSuiteScaffold::place() {
    side_widget_.measure({0.0, rect_.width, rect_.height, rect_.height});
    const double side = std::min(side_width(), rect_.width);
    const double bar_h = bar_widget_.height() * std::clamp(bar_.value(), 0.0, 1.0);
    content_ = {rect_.x + side, rect_.y, std::max(0.0, rect_.width - side), std::max(0.0, rect_.height - bar_h)};
    const double hidden = 1.0 - std::clamp(side_.value(), 0.0, 1.0);
    side_widget_.arrange({rect_.x - hidden * side_widget_.width(), rect_.y, side_widget_.width(), rect_.height});
    bar_widget_.set_width(rect_.width);
    bar_widget_.set_origin({rect_.x, rect_.y + rect_.height - bar_h});
    if (content_node_) {
        content_node_->measure(layout::Constraints::tight({content_.width, content_.height}));
        content_node_->arrange(content_);
    }
}

void NavigationSuiteScaffold::arrange(const render::Rect& rect) {
    rect_ = rect;
    place();
}

void NavigationSuiteScaffold::set_type(NavigationSuiteType type) {
    if (type == type_) {
        return;
    }
    const bool had_focus = focused();
    set_focused(false);
    type_ = type;
    bar_.animate_to(is_bar(type) ? 1.0 : 0.0);
    side_.animate_to(side_type(type) ? 1.0 : 0.0);
    retarget_side();
    cancel_interaction();
    if (had_focus) {
        set_focused(true);
    }
}

void NavigationSuiteScaffold::retarget_side() {
    if (side_type(type_)) {
        side_widget_.set_drawer(is_drawer(type_));
        side_widget_.set_expanded(labels_ && is_expanded(type_));
    }
}

void NavigationSuiteScaffold::set_labels(bool labels) {
    labels_ = labels;
    bar_widget_.set_labels(labels);
    retarget_side();
}

void NavigationSuiteScaffold::snap_to_targets() {
    for (motion::Animated* a : {&bar_, &side_}) {
        a->snap_to(a->target());
    }
    bar_widget_.snap_to_targets();
    side_widget_.snap_to_targets();
    if (rect_.width > 0.0) {
        place();
    }
}

void NavigationSuiteScaffold::update(const WindowAdaptiveInfo& info, const NavigationSuiteChoice& choice) {
    set_type(navigation_suite_type(info, choice));
}

void NavigationSuiteScaffold::sync(std::size_t index) {
    selected_ = index;
    if (bar_widget_.selected() != index) {
        bar_widget_.select(index);
    }
    if (side_widget_.selected() != index) {
        side_widget_.select(index);
    }
}

void NavigationSuiteScaffold::select(std::size_t index) {
    sync(index);
}

bool NavigationSuiteScaffold::focused() const {
    return bar_widget_.focused() || side_widget_.focused();
}

void NavigationSuiteScaffold::set_focused(bool focused) {
    if (!focused) {
        bar_widget_.set_focused(false);
        side_widget_.set_focused(false);
        return;
    }
    switch (active()) {
    case Active::bar:
        bar_widget_.set_focused(true);
        break;
    case Active::side:
        side_widget_.set_focused(true);
        break;
    case Active::none:
        break;
    }
}

bool NavigationSuiteScaffold::can_focus() const {
    switch (active()) {
    case Active::bar:
        return bar_widget_.can_focus();
    case Active::side:
        return side_widget_.can_focus();
    case Active::none:
        break;
    }
    return false;
}

render::Rect NavigationSuiteScaffold::focus_bounds() const {
    switch (active()) {
    case Active::bar:
        return bar_widget_.bounds();
    case Active::side:
        return side_widget_.bounds();
    case Active::none:
        break;
    }
    return rect_;
}

void NavigationSuiteScaffold::set_enabled(bool enabled) {
    enabled_ = enabled;
    bar_widget_.set_enabled(enabled);
    side_widget_.set_enabled(enabled);
}

void NavigationSuiteScaffold::set_density(tokens::Density density) {
    density_ = density;
    bar_widget_.set_density(density);
    side_widget_.set_density(density);
}

NavigationSuiteResponse NavigationSuiteScaffold::handle(const input::PointerEvent& e) {
    NavigationSuiteResponse r;
    const input::PointerEvent away{input::PointerAction::leave, 0.0, 0.0};
    const Active a = active();
    if (a == Active::bar) {
        if (bar_widget_.handle(e)) {
            r.selected = bar_widget_.selected();
        }
        r.consumed = bar_widget_.bounds().height > 0.0 && e.action != input::PointerAction::leave &&
                     e.y_dp >= bar_widget_.bounds().y && e.x_dp >= rect_.x && e.x_dp < rect_.x + rect_.width;
    } else {
        bar_widget_.handle(away);
    }
    if (a == Active::side) {
        if (side_widget_.handle(e)) {
            r.selected = side_widget_.selected();
        }
        r.consumed = r.consumed || (e.action != input::PointerAction::leave && e.x_dp < content_.x && e.x_dp >= rect_.x);
    } else {
        side_widget_.handle(away);
    }
    if (r.selected) {
        sync(*r.selected);
    }
    return r;
}

NavigationSuiteResponse NavigationSuiteScaffold::handle(const input::KeyEvent& e) {
    NavigationSuiteResponse r;
    switch (active()) {
    case Active::bar:
        if (bar_widget_.handle(e)) {
            r.selected = bar_widget_.selected();
            r.consumed = true;
        }
        break;
    case Active::side:
        if (side_widget_.handle(e)) {
            r.selected = side_widget_.selected();
            r.consumed = true;
        }
        break;
    case Active::none:
        break;
    }
    if (r.selected && *r.selected == selected_) {
        r.selected.reset();
    }
    if (r.selected) {
        sync(*r.selected);
    }
    return r;
}

void NavigationSuiteScaffold::cancel_interaction() {
    bar_widget_.cancel_interaction();
    side_widget_.cancel_interaction();
}

void NavigationSuiteScaffold::restyle() {
    style_stamp_.refresh();
    bar_widget_.restyle();
    side_widget_.restyle();
    bar_.set_spec(t().spring);
    side_.set_spec(t().spring);
    if (rect_.width > 0.0) {
        place();
    }
}

void NavigationSuiteScaffold::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    bar_.step(dt);
    side_.step(dt);
    bar_widget_.step(dt);
    side_widget_.step(dt);
    if (rect_.width > 0.0) {
        place();
    }
}

bool NavigationSuiteScaffold::animating() const {
    return !bar_.settled() || !side_.settled() || bar_widget_.animating() || side_widget_.animating();
}

void NavigationSuiteScaffold::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (side_visible()) {
        side_widget_.paint(canvas, scheme);
    }
    if (bar_visible()) {
        canvas.save();
        canvas.push_clip(rect_);
        bar_widget_.paint(canvas, scheme);
        canvas.pop_clip();
        canvas.restore();
    }
}

std::vector<AccessibleNode> NavigationSuiteScaffold::accessible_nodes() const {
    std::vector<AccessibleNode> out;
    switch (active()) {
    case Active::bar:
        out.push_back(bar_widget_.accessible());
        break;
    case Active::side:
        out.push_back(side_widget_.accessible());
        break;
    case Active::none:
        break;
    }
    return out;
}

}
