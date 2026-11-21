#include "lamina/widgets/pane_scaffold.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "lamina/tokens/drag.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/widget_set.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::PaneScaffoldTokens& t = tokens::pane_scaffold_tokens;
constexpr input::PointerEvent left_event{input::PointerAction::leave, 0.0, 0.0};
constexpr input::PointerEvent pressed_elsewhere{input::PointerAction::down, -std::numeric_limits<double>::infinity(),
                                                -std::numeric_limits<double>::infinity()};

double spring_seconds(const motion::SpringSpec& spec, double distance) {
    const double rate = spec.damping_ratio * std::sqrt(spec.stiffness);
    if (rate <= 0.0 || distance <= t.settle_distance) {
        return 0.0;
    }
    return std::log(distance / t.settle_distance) / rate;
}

enum class Motion { none, bounds, enter_left, enter_right, enter_left_delayed, enter_right_delayed, enter_expand, exit_left, exit_right, exit_shrink };
enum class Change { hidden, shown, entering, exiting };

}

double PaneExpansionAnchor::position_in(double total) const {
    switch (kind) {
    case Kind::proportion:
        return std::clamp(std::round(total * value), 0.0, total);
    case Kind::offset_from_start:
        return std::round(value);
    case Kind::offset_from_end:
        return total - std::round(value);
    }
    return 0.0;
}

std::string PaneExpansionAnchor::description(const PatternStrings& strings) const {
    switch (kind) {
    case Kind::proportion:
        return format_pattern(strings.proportion_anchor, std::to_string(static_cast<int>(value * 100.0)));
    case Kind::offset_from_start:
        return format_pattern(strings.start_offset_anchor, std::to_string(static_cast<int>(value)));
    case Kind::offset_from_end:
        return format_pattern(strings.end_offset_anchor, std::to_string(static_cast<int>(value)));
    }
    return {};
}

PaneExpansionState::PaneExpansionState(std::vector<PaneExpansionAnchor> anchors, std::optional<std::size_t> initial_anchor)
    : anchors_(std::move(anchors)), tracker_(tokens::velocity_tracking) {
    if (initial_anchor && *initial_anchor < anchors_.size()) {
        current_ = initial_anchor;
    }
}

void PaneExpansionState::set_first_pane_width(double width) {
    first_proportion_.reset();
    offset_.reset();
    first_width_ = width;
    current_.reset();
    settling_ = false;
}

void PaneExpansionState::set_first_pane_proportion(double proportion) {
    first_width_.reset();
    offset_.reset();
    first_proportion_ = std::clamp(proportion, 0.0, 1.0);
    current_.reset();
    settling_ = false;
}

void PaneExpansionState::clear() {
    first_width_.reset();
    first_proportion_.reset();
    offset_.reset();
    current_.reset();
    settling_ = false;
}

std::optional<PaneExpansionAnchor> PaneExpansionState::current_anchor() const {
    if (!current_) {
        return std::nullopt;
    }
    return anchors_[*current_];
}

std::optional<PaneExpansionAnchor> PaneExpansionState::next_anchor() const {
    if (anchors_.empty() || width_ <= 0.0) {
        return std::nullopt;
    }
    std::vector<std::size_t> order(anchors_.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return anchors_[a].position_in(width_) < anchors_[b].position_in(width_); });
    const double now = current();
    for (std::size_t i : order) {
        if (now < anchors_[i].position_in(width_)) {
            return anchors_[i];
        }
    }
    return anchors_[order.front()];
}

void PaneExpansionState::animate_to(const PaneExpansionAnchor& anchor, double velocity) {
    const auto it = std::find(anchors_.begin(), anchors_.end(), anchor);
    if (it == anchors_.end()) {
        return;
    }
    current_ = static_cast<std::size_t>(it - anchors_.begin());
    first_width_.reset();
    first_proportion_.reset();
    if (width_ <= 0.0) {
        return;
    }
    offset_ = current();
    target_ = anchor.position_in(width_);
    velocity_ = velocity;
    settling_ = *offset_ != target_ || velocity != 0.0;
}

void PaneExpansionState::snap_to(const PaneExpansionAnchor& anchor) {
    const auto it = std::find(anchors_.begin(), anchors_.end(), anchor);
    if (it == anchors_.end()) {
        return;
    }
    current_ = static_cast<std::size_t>(it - anchors_.begin());
    first_width_.reset();
    first_proportion_.reset();
    settling_ = false;
    if (width_ > 0.0) {
        offset_ = anchor.position_in(width_);
    }
}

void PaneExpansionState::nudge(double delta) {
    settling_ = false;
    first_width_.reset();
    first_proportion_.reset();
    current_.reset();
    offset_ = std::clamp(current() + delta, 0.0, width_);
}

void PaneExpansionState::on_measured(double width) {
    if (width == width_) {
        return;
    }
    width_ = width;
    if (!dragging_ && !settling_ && current_) {
        offset_ = anchors_[*current_].position_in(width);
    } else if (offset_) {
        offset_ = std::clamp(*offset_, 0.0, width);
    }
}

void PaneExpansionState::press(double x, double time_seconds) {
    dragging_ = true;
    settling_ = false;
    grab_ = x - current();
    tracker_.reset();
    tracker_.add(time_seconds, x);
}

void PaneExpansionState::drag(double x, double time_seconds) {
    if (!dragging_) {
        return;
    }
    tracker_.add(time_seconds, x);
    first_width_.reset();
    first_proportion_.reset();
    offset_ = std::clamp(x - grab_, 0.0, width_);
}

void PaneExpansionState::release(double time_seconds) {
    if (!dragging_) {
        return;
    }
    dragging_ = false;
    tracker_.release(time_seconds);
    if (anchors_.empty() || !offset_) {
        return;
    }
    const double velocity = tracker_.velocity(tokens::maximum_fling_velocity);
    const std::size_t i = closest(*offset_, velocity);
    current_ = i;
    target_ = anchors_[i].position_in(width_);
    velocity_ = velocity;
    settling_ = true;
}

void PaneExpansionState::cancel() {
    if (dragging_) {
        dragging_ = false;
    }
}

std::size_t PaneExpansionState::closest(double position, double velocity) const {
    std::size_t best = 0;
    double best_distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < anchors_.size(); ++i) {
        const double p = anchors_[i].position_in(width_);
        double distance = 0.0;
        if (velocity >= t.anchoring_velocity_threshold) {
            const double delta = p - position;
            distance = delta < 0.0 ? width_ - delta : delta;
        } else if (velocity <= -t.anchoring_velocity_threshold) {
            const double delta = position - p;
            distance = delta < 0.0 ? width_ - delta : delta;
        } else {
            distance = std::abs(position - p);
        }
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

void PaneExpansionState::step(double dt) {
    if (!settling_ || !offset_) {
        return;
    }
    const motion::SpringState s = motion::advance(t.anchoring, {*offset_, velocity_}, target_, dt);
    offset_ = std::clamp(s.value, 0.0, width_);
    velocity_ = s.velocity;
    if (motion::is_settled(s, target_)) {
        offset_ = target_;
        velocity_ = 0.0;
        settling_ = false;
    }
}

ThreePaneScaffold::ThreePaneScaffold(ThreePaneScaffoldOptions options, ThreePaneScaffoldNavigator& navigator)
    : options_(std::move(options)),
      navigator_(navigator),
      expansion_(options_.anchors, options_.initial_anchor),
      panes_{Pane{{0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}},
             Pane{{0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}},
             Pane{{0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}, {0.0, t.motion}}} {
    const std::array<const std::string*, pane_count> defaults{&options_.strings->primary_pane, &options_.strings->secondary_pane,
                                                              &options_.strings->tertiary_pane};
    for (std::size_t i = 0; i < pane_count; ++i) {
        if (options_.names[i].empty()) {
            options_.names[i] = *defaults[i];
        }
    }
}

layout::Size ThreePaneScaffold::measure(const layout::Constraints& c) const {
    const double w = std::isfinite(c.max_width) ? c.max_width : c.min_width;
    const double h = std::isfinite(c.max_height) ? c.max_height : c.min_height;
    return c.constrain({w, h});
}

WidgetSet* ThreePaneScaffold::widgets(PaneRole role) const {
    const auto& slot = options_.panes[index(role)];
    return slot ? slot->widgets : nullptr;
}

FocusManager* ThreePaneScaffold::pane_focus(PaneRole role) const {
    const auto& slot = options_.panes[index(role)];
    return slot ? slot->focus : nullptr;
}

bool ThreePaneScaffold::pane_shown(PaneRole role) const {
    return panes_[index(role)].shown && pane_rect(role).width > 0.0;
}

bool ThreePaneScaffold::pane_visible(PaneRole role) const {
    return panes_[index(role)].visible;
}

render::Rect ThreePaneScaffold::pane_rect(PaneRole role) const {
    const Pane& p = panes_[index(role)];
    return {p.x.value(), p.y.value(), std::max(0.0, p.width.value()), std::max(0.0, p.height.value())};
}

render::Rect ThreePaneScaffold::target_pane_rect(PaneRole role) const {
    return panes_[index(role)].target;
}

std::optional<render::Rect> ThreePaneScaffold::handle_rect() const {
    return handle_;
}

void ThreePaneScaffold::compute_targets(std::array<render::Rect, pane_count>& out, std::array<bool, pane_count>& shown) {
    const PaneScaffoldValue& v = navigator_.value();
    const PaneScaffoldDirective& d = navigator_.directive();
    std::vector<PaneRole> expanded;
    for (PaneRole role : options_.order.roles) {
        const bool has = options_.panes[index(role)].has_value();
        shown[index(role)] = has && v[role].shown();
        out[index(role)] = panes_[index(role)].target;
        if (has && v[role].kind == PaneAdaptedValue::Kind::expanded) {
            expanded.push_back(role);
        }
    }
    const double spacer = d.horizontal_partition_spacer;
    const double left = rect_.x;
    const double right = rect_.x + rect_.width;
    auto span = [&](PaneRole role, double from, double to) { out[index(role)] = {from, rect_.y, std::max(0.0, to - from), rect_.height}; };
    expansion_.on_measured(rect_.width);
    handle_.reset();
    const bool two = expanded.size() == 2;
    if (two && !expansion_.unspecified()) {
        const PaneRole a = expanded[0];
        const PaneRole b = expanded[1];
        const double half = spacer / 2.0;
        const bool moving = expansion_.dragging() || expansion_.settling();
        if (const std::optional<double> offset = expansion_.offset()) {
            const double o = *offset;
            if (o <= half) {
                span(a, left, left);
                span(b, moving ? left + o * 2.0 : left, right);
            } else if (o >= rect_.width - half) {
                span(a, left, moving ? left + o * 2.0 - rect_.width : right);
                span(b, right, right);
            } else {
                span(a, left, left + o - half);
                span(b, left + o + half, right);
            }
        } else {
            const double available = rect_.width;
            const double first = expansion_.first_pane_width().value_or(expansion_.first_pane_proportion().value_or(0.0) * (available - spacer));
            const bool by_proportion = expansion_.first_pane_proportion().has_value();
            if (first <= 0.0) {
                span(a, left, left);
                span(b, left, right);
            } else if (first >= available - spacer || (by_proportion && *expansion_.first_pane_proportion() >= 1.0)) {
                span(a, left, right);
                span(b, right, right);
            } else {
                span(a, left, left + first);
                span(b, left + first + spacer, right);
            }
        }
    } else if (!expanded.empty()) {
        std::vector<double> widths;
        for (PaneRole role : expanded) {
            widths.push_back(options_.preferred_widths[index(role)].value_or(d.default_pane_preferred_width));
        }
        const double allocatable = rect_.width - static_cast<double>(expanded.size() - 1) * spacer;
        double total = 0.0;
        for (double w : widths) {
            total += w;
        }
        if (allocatable > total) {
            std::size_t best = 0;
            for (std::size_t i = 1; i < expanded.size(); ++i) {
                if (t.priorities[index(expanded[i])] > t.priorities[index(expanded[best])]) {
                    best = i;
                }
            }
            widths[best] += allocatable - total;
        } else if (allocatable < total && total > 0.0) {
            for (double& w : widths) {
                w *= allocatable / total;
            }
        }
        double x = left;
        for (std::size_t i = 0; i < expanded.size(); ++i) {
            span(expanded[i], x, x + widths[i]);
            x += widths[i] + spacer;
        }
    }
    if (two && options_.expansion_handle) {
        const render::Rect a = out[index(expanded[0])];
        const render::Rect b = out[index(expanded[1])];
        double centre = (a.x + a.width + b.x) / 2.0;
        if (expansion_.offset() && (expansion_.dragging() || expansion_.settling())) {
            centre = left + *expansion_.offset();
        } else if (!expansion_.dragging()) {
            expansion_.on_offset_measured(centre - left);
        }
        centre = std::clamp(centre, left + spacer / 2.0, std::max(left + spacer / 2.0, right - spacer / 2.0));
        const tokens::DragHandleTokens& h = tokens::drag_handle_tokens;
        const double size = t.handle_touch_target;
        handle_ = render::Rect{centre - std::max(h.container_width, size) / 2.0, rect_.y + rect_.height / 2.0 - size / 2.0,
                               std::max(h.container_width, size), size};
    }
    for (PaneRole role : options_.order.roles) {
        const PaneAdaptedValue& pv = v[role];
        if (!shown[index(role)] || pv.kind != PaneAdaptedValue::Kind::reflowed || !options_.panes[index(pv.reflow_under)]) {
            continue;
        }
        render::Rect& anchor = out[index(pv.reflow_under)];
        const double vspacer = d.vertical_partition_spacer;
        const double h = std::min(d.default_pane_preferred_height, std::max(0.0, (anchor.height - vspacer) / 2.0));
        out[index(role)] = {anchor.x, anchor.y + anchor.height - h, anchor.width, h};
        anchor.height = std::max(0.0, anchor.height - h - vspacer);
    }
}

void ThreePaneScaffold::apply(const PaneScaffoldValue& value, bool animate) {
    std::array<render::Rect, pane_count> targets{};
    std::array<bool, pane_count> shown{};
    compute_targets(targets, shown);
    std::array<Change, pane_count> changes{};
    std::array<Motion, pane_count> motions{};
    for (std::size_t i = 0; i < pane_count; ++i) {
        const bool before = panes_[i].shown;
        changes[i] = before && shown[i] ? Change::shown : shown[i] ? Change::entering : before ? Change::exiting : Change::hidden;
        motions[i] = Motion::none;
    }
    const auto& order = options_.order.roles;
    const auto n = static_cast<std::ptrdiff_t>(pane_count);
    std::ptrdiff_t first_shown = n;
    std::ptrdiff_t last_shown = -1;
    std::ptrdiff_t first_entering = n;
    std::ptrdiff_t last_entering = -1;
    for (std::ptrdiff_t i = 0; i < n; ++i) {
        const Change c = changes[index(order[static_cast<std::size_t>(i)])];
        if (c == Change::shown) {
            first_shown = std::min(first_shown, i);
            last_shown = std::max(last_shown, i);
            motions[index(order[static_cast<std::size_t>(i)])] = Motion::bounds;
        } else if (c == Change::entering) {
            first_entering = std::min(first_entering, i);
            last_entering = std::max(last_entering, i);
        }
    }
    bool exit_right = false;
    bool exit_left = false;
    std::ptrdiff_t first_exit_right = n;
    std::ptrdiff_t last_exit_left = -1;
    for (std::ptrdiff_t i = 0; i < n; ++i) {
        const std::size_t k = index(order[static_cast<std::size_t>(i)]);
        if (changes[k] != Change::exiting) {
            continue;
        }
        const bool shown_left = first_shown < i;
        const bool entering_left = first_entering < i;
        const bool shown_right = last_shown > i;
        const bool entering_right = last_entering > i;
        if ((!shown_right && !entering_right) || (!shown_right && (shown_left || entering_left))) {
            exit_right = true;
            first_exit_right = std::min(first_exit_right, i);
            motions[k] = Motion::exit_right;
        } else if (!shown_left) {
            exit_left = true;
            last_exit_left = std::max(last_exit_left, i);
            motions[k] = Motion::exit_left;
        } else {
            motions[k] = Motion::exit_shrink;
        }
    }
    for (std::ptrdiff_t i = 0; i < n; ++i) {
        const std::size_t k = index(order[static_cast<std::size_t>(i)]);
        if (changes[k] != Change::entering) {
            continue;
        }
        const bool free_right = !(last_shown > i) && !(last_exit_left > i);
        const bool free_left = !(first_shown < i) && !(first_exit_right < i);
        if (free_right && !exit_right) {
            motions[k] = Motion::enter_right;
        } else if (free_left && !exit_left) {
            motions[k] = Motion::enter_left;
        } else if (free_right) {
            motions[k] = Motion::enter_right_delayed;
        } else if (free_left) {
            motions[k] = Motion::enter_left_delayed;
        } else {
            motions[k] = Motion::enter_expand;
        }
    }
    const double left = rect_.x;
    const double right = rect_.x + rect_.width;
    for (std::size_t k = 0; k < pane_count; ++k) {
        Pane& p = panes_[k];
        const render::Rect& r = targets[k];
        p.target = r;
        p.delay = 0.0;
        if (!animate) {
            p.x.snap_to(r.x);
            p.y.snap_to(r.y);
            p.width.snap_to(r.width);
            p.height.snap_to(r.height);
            p.shown = shown[k];
            p.visible = shown[k];
            p.placed = p.placed || shown[k];
            continue;
        }
        const render::Rect now = pane_rect(static_cast<PaneRole>(k));
        switch (motions[k]) {
        case Motion::bounds:
            p.x.animate_to(r.x);
            p.y.animate_to(r.y);
            p.width.animate_to(r.width);
            p.height.animate_to(r.height);
            break;
        case Motion::enter_right:
        case Motion::enter_right_delayed:
        case Motion::enter_left:
        case Motion::enter_left_delayed: {
            const bool from_right = motions[k] == Motion::enter_right || motions[k] == Motion::enter_right_delayed;
            p.x.snap_to(from_right ? right : left - r.width);
            p.y.snap_to(r.y);
            p.width.snap_to(r.width);
            p.height.snap_to(r.height);
            p.x.animate_to(r.x);
            if (motions[k] == Motion::enter_right_delayed || motions[k] == Motion::enter_left_delayed) {
                p.delay = t.delayed_ratio * spring_seconds(t.motion, std::abs(p.x.value() - r.x));
            }
            break;
        }
        case Motion::enter_expand:
            p.x.snap_to(r.x + r.width / 2.0);
            p.y.snap_to(r.y);
            p.width.snap_to(0.0);
            p.height.snap_to(r.height);
            p.x.animate_to(r.x);
            p.width.animate_to(r.width);
            break;
        case Motion::exit_right:
            p.x.animate_to(right);
            break;
        case Motion::exit_left:
            p.x.animate_to(left - now.width);
            break;
        case Motion::exit_shrink:
            p.x.animate_to(now.x + now.width / 2.0);
            p.width.animate_to(0.0);
            break;
        case Motion::none:
            break;
        }
        p.shown = shown[k];
        p.visible = shown[k] || changes[k] == Change::exiting || (p.visible && !shown[k] && !p.x.settled());
        p.placed = p.placed || shown[k];
    }
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (!panes_[k].shown) {
            if (FocusManager* f = pane_focus(static_cast<PaneRole>(k))) {
                f->clear_focus();
            }
            if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
                w->cancel_interaction();
            }
        }
    }
    applied_ = value;
    applied_directive_ = navigator_.directive();
}

bool ThreePaneScaffold::any_pane_focused() const {
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (const FocusManager* f = pane_focus(static_cast<PaneRole>(k)); f && f->has_focus()) {
            return true;
        }
    }
    return false;
}

void ThreePaneScaffold::focus_destination(bool had_focus) {
    const std::optional<PaneDestination> d = navigator_.current_destination();
    if (!had_focus || !d) {
        return;
    }
    FocusManager* dest = pane_focus(d->pane);
    if (!dest || !panes_[index(d->pane)].shown || dest->has_focus() || !dest->any_can_focus()) {
        return;
    }
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (FocusManager* f = pane_focus(static_cast<PaneRole>(k))) {
            f->clear_focus();
        }
    }
    dest->advance(FocusDirection::next);
}

void ThreePaneScaffold::sync_handle() {
    handle_painter_.set_state(handle_pressed_, expansion_.dragging());
}

void ThreePaneScaffold::arrange(const render::Rect& rect) {
    rect_ = rect;
    const PaneScaffoldValue& value = navigator_.value();
    if (!started_) {
        started_ = true;
        apply(value, false);
    } else if (!(value == applied_) || value.current != applied_.current) {
        const bool had_focus = any_pane_focused();
        const bool directive_only = !(navigator_.directive() == applied_directive_);
        apply(value, !directive_only || options_.animate_directive_changes);
        focus_destination(had_focus);
    } else {
        refresh_targets();
    }
    step(0.0);
}

void ThreePaneScaffold::refresh_targets() {
    std::array<render::Rect, pane_count> targets{};
    std::array<bool, pane_count> shown{};
    compute_targets(targets, shown);
    for (std::size_t k = 0; k < pane_count; ++k) {
        Pane& p = panes_[k];
        if (!p.shown || !shown[k]) {
            continue;
        }
        const render::Rect& r = targets[k];
        p.target = r;
        const bool moving = !p.x.settled() || !p.y.settled() || !p.width.settled() || !p.height.settled();
        if (moving) {
            p.x.animate_to(r.x);
            p.y.animate_to(r.y);
            p.width.animate_to(r.width);
            p.height.animate_to(r.height);
        } else {
            p.x.snap_to(r.x);
            p.y.snap_to(r.y);
            p.width.snap_to(r.width);
            p.height.snap_to(r.height);
        }
    }
}

bool ThreePaneScaffold::back() {
    if (!navigator_.can_navigate_back()) {
        return false;
    }
    navigator_.navigate_back();
    return true;
}

std::optional<input::Cursor> ThreePaneScaffold::cursor() const {
    if (handle_ && enabled() && (handle_hovered_ || expansion_.dragging())) {
        return input::Cursor::resize_horizontal;
    }
    return std::nullopt;
}

void ThreePaneScaffold::set_focused(bool focused) {
    focus_.set_focused(focused && can_focus());
}

bool ThreePaneScaffold::can_focus() const {
    return enabled() && handle_.has_value();
}

render::Rect ThreePaneScaffold::bounds_for_focus() const {
    return handle_.value_or(render::Rect{0.0, 0.0, 0.0, 0.0});
}

void ThreePaneScaffold::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
            w->set_enabled(enabled);
        }
    }
    if (!enabled) {
        set_focused(false);
        cancel_interaction();
    }
}

PaneScaffoldResponse ThreePaneScaffold::handle(const input::PointerEvent& e) {
    PaneScaffoldResponse r;
    if (!enabled()) {
        return r;
    }
    focus_.release_on_press(e);
    const double time = e.time_seconds > 0.0 ? e.time_seconds : clock_;
    if (expansion_.dragging()) {
        if (e.action == input::PointerAction::up || e.action == input::PointerAction::leave) {
            expansion_.release(time);
            handle_pressed_ = false;
        } else {
            expansion_.drag(e.x_dp - rect_.x, time);
        }
        handle_hovered_ = handle_ && e.action != input::PointerAction::leave && contains(*handle_, e.x_dp, e.y_dp);
        sync_handle();
        r.consumed = true;
        return r;
    }
    handle_hovered_ = handle_ && e.action != input::PointerAction::leave && contains(*handle_, e.x_dp, e.y_dp);
    if (handle_hovered_ && e.action == input::PointerAction::down) {
        handle_pressed_ = true;
        expansion_.press(e.x_dp - rect_.x, time);
        sync_handle();
        for (std::size_t k = 0; k < pane_count; ++k) {
            if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
                w->handle(pressed_elsewhere);
            }
        }
        r.consumed = true;
        return r;
    }
    for (std::size_t k = 0; k < pane_count; ++k) {
        WidgetSet* w = widgets(static_cast<PaneRole>(k));
        if (!w) {
            continue;
        }
        const bool inside = panes_[k].shown && e.action != input::PointerAction::leave && !handle_hovered_ &&
                            contains(pane_rect(static_cast<PaneRole>(k)), e.x_dp, e.y_dp);
        if (inside) {
            r.consumed = w->handle(e) || r.consumed;
        } else if (e.action == input::PointerAction::down) {
            w->handle(pressed_elsewhere);
        } else {
            w->handle(left_event);
        }
    }
    return r;
}

PaneScaffoldResponse ThreePaneScaffold::handle(const input::KeyEvent& e) {
    PaneScaffoldResponse r;
    if (!enabled()) {
        return r;
    }
    if (focused() && handle_) {
        if (e.key == input::Key::left || e.key == input::Key::right) {
            expansion_.nudge(e.key == input::Key::right ? t.keyboard_step : -t.keyboard_step);
            r.consumed = true;
            return r;
        }
        if ((e.key == input::Key::enter || e.key == input::Key::space) && !e.repeat) {
            if (const std::optional<PaneExpansionAnchor> next = expansion_.next_anchor()) {
                expansion_.animate_to(*next);
            }
            r.consumed = true;
            return r;
        }
    }
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (!panes_[k].shown) {
            continue;
        }
        if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
            r.consumed = w->handle(e) || r.consumed;
        }
    }
    if (!r.consumed && e.key == input::Key::escape && back()) {
        r.navigated_back = true;
        r.consumed = true;
    }
    return r;
}

void ThreePaneScaffold::handle(const input::TextEvent& e) {
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (!panes_[k].shown) {
            continue;
        }
        if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
            w->handle(e);
        }
    }
}

void ThreePaneScaffold::cancel_interaction() {
    expansion_.cancel();
    handle_pressed_ = false;
    handle_hovered_ = false;
    sync_handle();
    for (std::size_t k = 0; k < pane_count; ++k) {
        if (WidgetSet* w = widgets(static_cast<PaneRole>(k))) {
            w->cancel_interaction();
        }
    }
}

void ThreePaneScaffold::step(double dt) {
    clock_ += dt;
    expansion_.step(dt);
    if (expansion_.settling() || expansion_.dragging()) {
        std::array<render::Rect, pane_count> targets{};
        std::array<bool, pane_count> shown{};
        compute_targets(targets, shown);
        for (std::size_t k = 0; k < pane_count; ++k) {
            panes_[k].target = targets[k];
            panes_[k].x.snap_to(targets[k].x);
            panes_[k].width.snap_to(targets[k].width);
        }
    }
    for (Pane& p : panes_) {
        if (p.delay > 0.0) {
            p.delay = std::max(0.0, p.delay - dt);
            continue;
        }
        p.x.step(dt);
        p.y.step(dt);
        p.width.step(dt);
        p.height.step(dt);
        if (!p.shown && p.visible && p.x.settled() && p.width.settled()) {
            p.visible = false;
        }
    }
    handle_painter_.step(dt);
    focus_.step(dt);
    enabled_.step(dt);
    for (std::size_t k = 0; k < pane_count; ++k) {
        const auto& slot = options_.panes[k];
        if (!slot || !slot->node || !panes_[k].visible) {
            continue;
        }
        const render::Rect r = pane_rect(static_cast<PaneRole>(k));
        slot->node->measure(layout::Constraints::tight({r.width, r.height}));
        slot->node->arrange(r);
        if (slot->widgets) {
            slot->widgets->step(dt);
        }
    }
}

bool ThreePaneScaffold::animating() const {
    for (std::size_t k = 0; k < pane_count; ++k) {
        const Pane& p = panes_[k];
        if (!p.x.settled() || !p.y.settled() || !p.width.settled() || !p.height.settled() || p.delay > 0.0) {
            return true;
        }
        if (const WidgetSet* w = widgets(static_cast<PaneRole>(k)); w && p.visible && w->animating()) {
            return true;
        }
    }
    return expansion_.animating() || !handle_painter_.settled() || !focus_.settled() || !enabled_.settled();
}

void ThreePaneScaffold::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    for (PaneRole role : options_.order.roles) {
        const std::size_t k = index(role);
        const WidgetSet* w = widgets(role);
        if (!w || !panes_[k].visible) {
            continue;
        }
        const render::Rect r = pane_rect(role);
        if (r.width <= 0.0 || r.height <= 0.0) {
            continue;
        }
        canvas.save();
        canvas.push_clip(r);
        w->paint(canvas, scheme);
        canvas.pop_clip();
        canvas.restore();
    }
    if (handle_) {
        const render::Point centre{handle_->x + handle_->width / 2.0, handle_->y + handle_->height / 2.0};
        handle_painter_.paint(canvas, scheme, centre, enabled_.amount() * (1.0 - tokens::disabled_alpha) + tokens::disabled_alpha);
        const render::Rect pill = handle_painter_.rect(centre);
        focus_.paint(canvas, scheme, pill, handle_painter_.radius(pill));
    }
}

std::vector<AccessibleNode> ThreePaneScaffold::accessible_nodes() const {
    std::vector<AccessibleNode> out;
    for (PaneRole role : options_.order.roles) {
        const WidgetSet* w = widgets(role);
        if (!w || !panes_[index(role)].shown || pane_rect(role).width <= 0.0) {
            continue;
        }
        AccessibleNode group{AccessibleRole::group, options_.names[index(role)], pane_rect(role)};
        group.children = w->accessible_nodes();
        out.push_back(std::move(group));
    }
    if (handle_) {
        const PatternStrings& s = *options_.strings;
        AccessibleNode node{AccessibleRole::separator, s.pane_expansion_drag_handle, *handle_,
                            {.enabled = enabled(), .focusable = can_focus(), .focused = focused()}};
        if (const std::optional<PaneExpansionAnchor> a = expansion_.current_anchor()) {
            node.value = format_pattern(s.pane_split_state, a->description(s));
        }
        if (const std::optional<PaneExpansionAnchor> next = expansion_.next_anchor()) {
            node.description = format_pattern(s.pane_split_action, next->description(s));
        }
        out.push_back(std::move(node));
    }
    return out;
}

}
