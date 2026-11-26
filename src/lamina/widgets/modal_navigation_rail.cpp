#include "lamina/style/style.hpp"
#include "lamina/widgets/modal_navigation_rail.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/render/path.hpp"
#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/scrim.hpp"

namespace lamina::widgets {

namespace {

const tokens::NavigationRailTokens& rail_t() {
    return current_style().navigation_rail;
}
const tokens::ModalNavigationRailTokens& t() {
    return current_style().modal_navigation_rail;
}

NavigationRailOptions rail_options(NavigationRailOptions o, bool hide) {
    if (hide) {
        o.expanded = true;
    }
    return o;
}

}

ModalNavigationRail::ModalNavigationRail(ModalNavigationRailOptions options, const text::FontStack& fonts)
    : options_(std::move(options)),
      rail_(rail_options(options_.rail, options_.hide_on_collapse), fonts),
      slide_(RailValue::collapsed, {[](double distance) { return distance * t().positional_fraction; }, t().velocity_threshold},
             current_style().gesture.drag),
      scrim_(0.0, t().scrim) {}

layout::Size ModalNavigationRail::measure(const layout::Constraints& c) {
    const double reserved = options_.hide_on_collapse ? 0.0 : rail_t().collapsed_width;
    return c.constrain({reserved, std::isfinite(c.max_height) ? c.max_height : c.min_height});
}

void ModalNavigationRail::arrange(const render::Rect& rect) {
    slot_ = rect;
    if (area_.width <= 0.0) {
        area_ = rect;
    }
    layout_rail();
}

void ModalNavigationRail::set_area(const render::Rect& area) {
    area_ = area;
    layout_rail();
}

void ModalNavigationRail::layout_rail() {
    rail_.measure({0.0, std::max(area_.width, slot_.width), 0.0, slot_.height});
    if (options_.hide_on_collapse) {
        slide_.set_anchors({{RailValue::collapsed, -rail_.width()}, {RailValue::expanded, 0.0}}, slide_.target_value());
    }
    const double x = slot_.x + (options_.hide_on_collapse ? slide_.offset() : 0.0);
    rail_.arrange({x, slot_.y, rail_.width(), slot_.height});
}

bool ModalNavigationRail::showing() const {
    if (options_.hide_on_collapse) {
        return slide_.positioned() && (slide_.target_value() == RailValue::expanded || !slide_.idle() || slide_.offset() > -rail_.width());
    }
    return target_expanded_ || rail_.width() > rail_t().collapsed_width + 0.5 || scrim_.value() > 0.0;
}

void ModalNavigationRail::expand(input::InputSource source) {
    target_expanded_ = true;
    if (options_.hide_on_collapse) {
        slide_.animate_to(RailValue::expanded, t().slide);
    } else {
        rail_.set_expanded(true);
    }
    if (source == input::InputSource::keyboard) {
        rail_.set_focused(true);
        claim_focus_ = true;
    }
}

void ModalNavigationRail::collapse() {
    target_expanded_ = false;
    claim_focus_ = false;
    if (options_.hide_on_collapse) {
        slide_.animate_to(RailValue::collapsed, t().slide);
        rail_.set_focused(false);
    } else {
        rail_.set_expanded(false);
    }
    rail_.cancel_interaction();
}

ModalRailResponse ModalNavigationRail::handle(const input::PointerEvent& e) {
    ModalRailResponse r;
    const bool in_rail = e.action != input::PointerAction::leave && contains(rail_.bounds(), e.x_dp, e.y_dp);
    if (!showing()) {
        r.selected = rail_.handle(e);
        r.consumed = in_rail;
        return r;
    }
    const bool in_area = e.action != input::PointerAction::leave && contains(area_, e.x_dp, e.y_dp);
    switch (e.action) {
    case input::PointerAction::down:
        scrim_pressed_ = in_area && !in_rail;
        tracking_ = false;
        if (options_.hide_on_collapse && in_rail) {
            slide_.press(e.x_dp, e.time_seconds);
            tracking_ = true;
        }
        break;
    case input::PointerAction::move:
        if (tracking_) {
            const bool was = slide_.dragging();
            if (slide_.move(e.x_dp, e.time_seconds)) {
                if (!was) {
                    rail_.cancel_interaction();
                }
                layout_rail();
                r.consumed = true;
                return r;
            }
        }
        break;
    case input::PointerAction::up:
        tracking_ = false;
        if (slide_.dragging()) {
            const std::optional<double> v = slide_.release(e.time_seconds);
            if (slide_.fling(v.value_or(0.0), t().slide) == RailValue::collapsed) {
                target_expanded_ = false;
                rail_.set_focused(false);
                r.collapsed = true;
            }
            r.consumed = true;
            return r;
        }
        slide_.cancel_press();
        if (scrim_pressed_ && in_area && !in_rail) {
            scrim_pressed_ = false;
            collapse();
            r.collapsed = true;
            r.consumed = true;
            return r;
        }
        scrim_pressed_ = false;
        break;
    case input::PointerAction::leave:
        tracking_ = false;
        slide_.cancel_press();
        break;
    }
    r.selected = rail_.handle(e);
    r.consumed = in_area || in_rail;
    return r;
}

ModalRailResponse ModalNavigationRail::handle(const input::KeyEvent& e) {
    ModalRailResponse r;
    if (!showing()) {
        r.selected = rail_.handle(e);
        r.consumed = r.selected;
        return r;
    }
    r.consumed = true;
    if (e.key == input::Key::escape && target_expanded_) {
        collapse();
        r.collapsed = true;
        return r;
    }
    if (e.key == input::Key::tab && !e.ctrl) {
        rail_.set_focused(true);
        return r;
    }
    r.selected = rail_.handle(e);
    return r;
}

void ModalNavigationRail::cancel_interaction() {
    rail_.cancel_interaction();
    if (slide_.dragging()) {
        slide_.cancel_press();
        slide_.fling(0.0, t().slide);
    }
    slide_.cancel_press();
    tracking_ = false;
    scrim_pressed_ = false;
}

void ModalNavigationRail::restyle() {
    style_stamp_.refresh();
    rail_.restyle();
    slide_.set_gesture(current_style().gesture.drag);
    scrim_.set_spec(t().scrim);
    layout_rail();
}

void ModalNavigationRail::step(double dt) {
    if (style_stamp_.stale()) {
        restyle();
    }
    if (claim_focus_) {
        claim_focus_ = false;
        if (target_expanded_ && !rail_.focused()) {
            rail_.set_focused(true);
        }
    }
    slide_.step(dt);
    scrim_.animate_to(target_expanded_ ? 1.0 : 0.0);
    scrim_.step(dt);
    rail_.step(dt);
    layout_rail();
}

bool ModalNavigationRail::animating() const {
    return !slide_.idle() || slide_.pressed() || !scrim_.settled() || rail_.animating();
}

void ModalNavigationRail::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!showing()) {
        if (!options_.hide_on_collapse) {
            rail_.paint(canvas, scheme);
        }
        return;
    }
    if (!options_.hide_on_collapse) {
        canvas.fill_rect({slot_.x, slot_.y, rail_t().collapsed_width, slot_.height}, scheme[rail_t().container]);
    }
    canvas.save();
    canvas.push_clip(area_);
    paint_scrim(canvas, area_, scheme, scrim_.value());
    const render::Rect b = rail_.bounds();
    const double stretch = options_.hide_on_collapse ? std::max(0.0, b.x - slot_.x) : 0.0;
    const render::Rect container{b.x - stretch, b.y, b.width + stretch, b.height};
    const double radius = current_style().shape.radius(t().expanded_shape, container.width, container.height);
    canvas.fill_path(render::rounded_rect_path(container, render::uniform_radii(radius)), scheme[t().container]);
    rail_.paint_items(canvas, scheme);
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> ModalNavigationRail::accessible() const {
    if (!showing()) {
        if (options_.hide_on_collapse) {
            return std::nullopt;
        }
        return rail_.accessible();
    }
    AccessibleNode node = rail_.accessible();
    if (node.name.empty()) {
        node.name = options_.strings->navigation_rail_pane_title;
    }
    node.state.modal = true;
    node.children.push_back({AccessibleRole::button, options_.strings->close_rail, area_});
    return node;
}

}
