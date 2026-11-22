#include "lamina/widgets/navigation_suite.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/navigation_suite.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::NavigationSuiteTokens& t = tokens::navigation_suite_tokens;

std::vector<DrawerEntry> drawer_entries(const std::vector<NavigationItem>& items) {
    std::vector<DrawerEntry> out;
    for (const NavigationItem& item : items) {
        out.push_back({DrawerEntryKind::item, item.label.empty() ? item.accessible_name : item.label, item.icon, item.badge, item.enabled});
    }
    return out;
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
      rail_widget_(NavigationRailOptions{options.items, options.selected, options.type == NavigationSuiteType::wide_navigation_rail_expanded,
                                         nullptr, options.accessible_name, options.density, options.labels},
                   fonts),
      drawer_widget_(DrawerSheetOptions{drawer_entries(options.items), options.selected, DrawerSheetStyle::standard, options.strings,
                                        options.accessible_name, options.density},
                     fonts),
      bar_(is_bar(options.type) ? 1.0 : 0.0, t.spring),
      side_(!is_bar(options.type) && options.type != NavigationSuiteType::none ? 1.0 : 0.0, t.spring),
      drawer_(is_drawer(options.type) ? 1.0 : 0.0, t.spring),
      labels_(options.labels),
      density_(options.density) {}

bool NavigationSuiteScaffold::is_bar(NavigationSuiteType type) {
    return type == NavigationSuiteType::short_navigation_bar_compact || type == NavigationSuiteType::short_navigation_bar_medium;
}

bool NavigationSuiteScaffold::is_drawer(NavigationSuiteType type) {
    return type == NavigationSuiteType::navigation_drawer;
}

NavigationSuiteScaffold::Active NavigationSuiteScaffold::active() const {
    if (is_bar(type_)) {
        return Active::bar;
    }
    if (is_drawer(type_)) {
        return Active::drawer;
    }
    return type_ == NavigationSuiteType::none ? Active::none : Active::rail;
}

bool NavigationSuiteScaffold::bar_visible() const {
    return bar_.value() > 0.0;
}

bool NavigationSuiteScaffold::rail_visible() const {
    return side_.value() > 0.0 && drawer_.value() < 1.0;
}

bool NavigationSuiteScaffold::drawer_visible() const {
    return side_.value() > 0.0 && drawer_.value() > 0.0;
}

layout::Size NavigationSuiteScaffold::measure(const layout::Constraints& c) const {
    const double w = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : c.min_height;
    return c.constrain({w, h});
}

double NavigationSuiteScaffold::side_width() const {
    const double rail = rail_widget_.width();
    const double drawer = t.drawer_width;
    return std::max(0.0, side_.value()) * motion::lerp(rail, drawer, std::clamp(drawer_.value(), 0.0, 1.0));
}

void NavigationSuiteScaffold::place() {
    rail_widget_.measure({0.0, rect_.width, rect_.height, rect_.height});
    const double side = std::min(side_width(), rect_.width);
    const double bar_h = bar_widget_.height() * std::clamp(bar_.value(), 0.0, 1.0);
    content_ = {rect_.x + side, rect_.y, std::max(0.0, rect_.width - side), std::max(0.0, rect_.height - bar_h)};
    const double hidden = 1.0 - std::clamp(side_.value(), 0.0, 1.0);
    rail_widget_.arrange({rect_.x - hidden * rail_widget_.width(), rect_.y, rail_widget_.width(), rect_.height});
    drawer_widget_.measure({0.0, t.drawer_width, rect_.height, rect_.height});
    drawer_widget_.arrange({rect_.x - hidden * t.drawer_width, rect_.y, t.drawer_width, rect_.height});
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
    side_.animate_to(!is_bar(type) && type != NavigationSuiteType::none ? 1.0 : 0.0);
    drawer_.animate_to(is_drawer(type) ? 1.0 : 0.0);
    if (type == NavigationSuiteType::wide_navigation_rail_collapsed || type == NavigationSuiteType::wide_navigation_rail_expanded) {
        rail_widget_.set_expanded(type == NavigationSuiteType::wide_navigation_rail_expanded);
    }
    cancel_interaction();
    if (had_focus) {
        set_focused(true);
    }
}

void NavigationSuiteScaffold::set_labels(bool labels) {
    labels_ = labels;
    bar_widget_.set_labels(labels);
    rail_widget_.set_labels(labels);
}

void NavigationSuiteScaffold::snap_to_targets() {
    for (motion::Animated* a : {&bar_, &side_, &drawer_}) {
        a->snap_to(a->target());
    }
    bar_widget_.snap_to_targets();
    rail_widget_.snap_to_targets();
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
    if (rail_widget_.selected() != index) {
        rail_widget_.select(index);
    }
    if (drawer_widget_.selected() != index) {
        drawer_widget_.select(index);
    }
}

void NavigationSuiteScaffold::select(std::size_t index) {
    sync(index);
}

bool NavigationSuiteScaffold::focused() const {
    return bar_widget_.focused() || rail_widget_.focused() || drawer_widget_.focused();
}

void NavigationSuiteScaffold::set_focused(bool focused) {
    if (!focused) {
        bar_widget_.set_focused(false);
        rail_widget_.set_focused(false);
        drawer_widget_.set_focused(false);
        return;
    }
    switch (active()) {
    case Active::bar:
        bar_widget_.set_focused(true);
        break;
    case Active::rail:
        rail_widget_.set_focused(true);
        break;
    case Active::drawer:
        drawer_widget_.set_focused(true);
        break;
    case Active::none:
        break;
    }
}

bool NavigationSuiteScaffold::can_focus() const {
    switch (active()) {
    case Active::bar:
        return bar_widget_.can_focus();
    case Active::rail:
        return rail_widget_.can_focus();
    case Active::drawer:
        return drawer_widget_.can_focus();
    case Active::none:
        break;
    }
    return false;
}

render::Rect NavigationSuiteScaffold::focus_bounds() const {
    switch (active()) {
    case Active::bar:
        return bar_widget_.bounds();
    case Active::rail:
        return rail_widget_.bounds();
    case Active::drawer:
        return drawer_widget_.bounds();
    case Active::none:
        break;
    }
    return rect_;
}

void NavigationSuiteScaffold::set_enabled(bool enabled) {
    enabled_ = enabled;
    bar_widget_.set_enabled(enabled);
    rail_widget_.set_enabled(enabled);
    drawer_widget_.set_enabled(enabled);
}

void NavigationSuiteScaffold::set_density(tokens::Density density) {
    density_ = density;
    bar_widget_.set_density(density);
    rail_widget_.set_density(density);
    drawer_widget_.set_density(density);
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
    if (a == Active::rail) {
        if (rail_widget_.handle(e)) {
            r.selected = rail_widget_.selected();
        }
        r.consumed = r.consumed || (e.action != input::PointerAction::leave && e.x_dp < content_.x && e.x_dp >= rect_.x);
    } else {
        rail_widget_.handle(away);
    }
    if (a == Active::drawer) {
        const DrawerResponse d = drawer_widget_.handle(e);
        if (d.selected) {
            r.selected = d.selected;
        }
        r.consumed = r.consumed || d.consumed || (e.action != input::PointerAction::leave && e.x_dp < content_.x && e.x_dp >= rect_.x);
    } else {
        drawer_widget_.handle(away);
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
    case Active::rail:
        if (rail_widget_.handle(e)) {
            r.selected = rail_widget_.selected();
            r.consumed = true;
        }
        break;
    case Active::drawer: {
        const DrawerResponse d = drawer_widget_.handle(e);
        r.selected = d.selected;
        r.consumed = d.consumed;
        break;
    }
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
    rail_widget_.cancel_interaction();
    drawer_widget_.cancel_interaction();
}

void NavigationSuiteScaffold::step(double dt) {
    bar_.step(dt);
    side_.step(dt);
    drawer_.step(dt);
    bar_widget_.step(dt);
    rail_widget_.step(dt);
    drawer_widget_.step(dt);
    if (rect_.width > 0.0) {
        place();
    }
}

bool NavigationSuiteScaffold::animating() const {
    return !bar_.settled() || !side_.settled() || !drawer_.settled() || bar_widget_.animating() || rail_widget_.animating() ||
           drawer_widget_.animating();
}

void NavigationSuiteScaffold::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double drawer = std::clamp(drawer_.value(), 0.0, 1.0);
    if (rail_visible()) {
        if (drawer > 0.0) {
            const color::ColorScheme faded = color::faded(scheme, 1.0 - drawer);
            rail_widget_.paint(canvas, faded);
        } else {
            rail_widget_.paint(canvas, scheme);
        }
    }
    if (drawer_visible()) {
        if (drawer < 1.0) {
            const color::ColorScheme faded = color::faded(scheme, drawer);
            drawer_widget_.paint(canvas, faded);
        } else {
            drawer_widget_.paint(canvas, scheme);
        }
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
    case Active::rail:
        out.push_back(rail_widget_.accessible());
        break;
    case Active::drawer:
        out.push_back(drawer_widget_.accessible());
        break;
    case Active::none:
        break;
    }
    return out;
}

}
