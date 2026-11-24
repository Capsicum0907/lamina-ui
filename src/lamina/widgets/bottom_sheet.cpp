#include "lamina/style/style.hpp"
#include "lamina/widgets/bottom_sheet.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/tokens/interaction.hpp"
#include "lamina/widgets/scrim.hpp"
#include "lamina/widgets/surface.hpp"

namespace lamina::widgets {

namespace {

const tokens::BottomSheetTokens& t() {
    return current_style().bottom_sheet;
}
const tokens::SheetMotion& motion_spec() {
    return current_style().bottom_sheet_motion;
}

constexpr input::PointerEvent left{input::PointerAction::leave, 0.0, 0.0};

motion::DragThresholds sheet_thresholds() {
    return {[](double) { return t().positional_threshold; }, t().velocity_threshold};
}

}

render::Rect SheetDragHandle::handle_rect() const {
    return {bounds_.x + (bounds_.width - t().drag_handle_width) / 2.0, bounds_.y + t().drag_handle_vertical_padding, t().drag_handle_width,
            t().drag_handle_height};
}

void SheetDragHandle::set_focused(bool focused) {
    focus_.set_focused(focused && shown_);
    retarget();
}

void SheetDragHandle::set_shown(bool shown) {
    shown_ = shown;
    if (!shown) {
        press_ = {};
        focus_.set_focused(false);
        retarget();
    }
}

bool SheetDragHandle::handle(const input::PointerEvent& e) {
    if (!shown_) {
        return false;
    }
    focus_.release_on_press(e);
    const bool clicked = press_.update(e, bounds_);
    retarget();
    return clicked;
}

void SheetDragHandle::cancel_interaction() {
    press_ = {};
    retarget();
}

void SheetDragHandle::retarget() {
    state_layer_.animate_to(focus_.state_layer_opacity(press_.state_layer_opacity()));
}

void SheetDragHandle::step(double dt) {
    state_layer_.step(dt);
    focus_.step(dt);
}

void SheetDragHandle::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double layer = std::clamp(state_layer_.value(), 0.0, 1.0);
    const double radius = tokens::corner_radius(tokens::CornerSize::full, bounds_.width, bounds_.height);
    if (layer > 0.0) {
        canvas.fill_path(render::rounded_rect_path(bounds_, render::uniform_radii(radius)),
                         color::with_alpha(scheme[t().drag_handle_state_layer], layer));
    }
    const render::Rect h = handle_rect();
    canvas.fill_path(render::rounded_rect_path(h, render::uniform_radii(std::min(tokens::corner_radius(t().drag_handle_corner, h.width, h.height),
                                                                                  h.height / 2.0))),
                     scheme[t().drag_handle]);
    focus_.paint(canvas, scheme, bounds_, radius);
}

BottomSheet::BottomSheet(BottomSheetOptions options)
    : options_(std::move(options)),
      handle_(options_.strings->drag_handle),
      content_(options_.content),
      drag_(options_.initial, sheet_thresholds(), tokens::drag_gesture),
      scrim_(0.0, motion_spec().scrim) {
    if (options_.accessible_name.empty()) {
        options_.accessible_name = options_.strings->bottom_sheet_pane_title;
    }
    if (options_.kind == BottomSheetKind::modal) {
        options_.skip_hidden = false;
    }
    if (options_.drag_handle) {
        own_focus_.add(handle_);
    }
}

double BottomSheet::handle_height() const {
    return options_.drag_handle ? t().drag_handle_height + 2.0 * t().drag_handle_vertical_padding : 0.0;
}

void BottomSheet::set_area(const render::Rect& area) {
    area_ = area;
    layout_sheet();
    update_anchors();
    place_parts();
    update_handle();
}

void BottomSheet::layout_sheet() {
    sheet_width_ = std::min(area_.width, t().max_width);
    const double natural = handle_height() + content_.measure(sheet_width_);
    sheet_height_ = std::min(natural, area_.height);
    if (!modal()) {
        sheet_height_ = std::max(sheet_height_, std::min(options_.peek_height, area_.height));
    }
}

void BottomSheet::update_anchors() {
    using Anchor = motion::AnchoredDraggable<SheetValue>::Anchor;
    const double full = area_.height;
    const double h = sheet_height_;
    std::vector<Anchor> anchors;
    if (modal()) {
        anchors.push_back({SheetValue::hidden, full});
        if (!options_.skip_partially_expanded && h != 0.0) {
            anchors.push_back({SheetValue::partially_expanded,
                               full - std::min(full * t().partial_screen_fraction, h * t().partial_sheet_fraction)});
        }
        if (h != 0.0) {
            anchors.push_back({SheetValue::expanded, std::max(0.0, full - h)});
        }
    } else {
        const double peek = options_.peek_height;
        if (!options_.skip_partially_expanded && peek > 0.0 && peek != h) {
            anchors.push_back({SheetValue::partially_expanded, full - peek});
        }
        if (h == 0.0 || peek == 0.0 || !options_.skip_hidden) {
            anchors.push_back({SheetValue::hidden, full});
        }
        if (h > 0.0) {
            anchors.push_back({SheetValue::expanded, full - h});
        }
    }
    auto has = [&](SheetValue v) { return std::any_of(anchors.begin(), anchors.end(), [v](const Anchor& a) { return a.value == v; }); };
    SheetValue target = drag_.target_value();
    switch (target) {
    case SheetValue::hidden:
        if (!has(SheetValue::hidden)) {
            target = has(SheetValue::partially_expanded) ? SheetValue::partially_expanded : SheetValue::expanded;
        }
        break;
    case SheetValue::partially_expanded:
        if (!has(SheetValue::partially_expanded)) {
            target = has(SheetValue::expanded) ? SheetValue::expanded : SheetValue::hidden;
        }
        break;
    case SheetValue::expanded:
        if (!has(SheetValue::expanded)) {
            target = SheetValue::hidden;
        }
        break;
    }
    drag_.set_anchors(std::move(anchors), target);
}

render::Rect BottomSheet::bounds() const {
    return {area_.x + (area_.width - sheet_width_) / 2.0, area_.y + drag_.offset(), sheet_width_, sheet_height_};
}

render::Rect BottomSheet::body_area() const {
    return {area_.x, area_.y, area_.width, std::max(0.0, area_.height - options_.peek_height)};
}

double BottomSheet::visible_height() const {
    return std::max(0.0, area_.height - drag_.offset());
}

void BottomSheet::place_parts() {
    const render::Rect s = bounds();
    handle_.set_bounds({s.x + (s.width - t().drag_handle_width) / 2.0, s.y, t().drag_handle_width, handle_height()});
    content_.place({s.x, s.y + handle_height(), s.width, std::max(0.0, s.height - handle_height())});
}

void BottomSheet::update_handle() {
    const bool shown = options_.drag_handle && visible() && drag_.target_value() != SheetValue::hidden;
    handle_.set_shown(shown);
    const PatternStrings& s = *options_.strings;
    switch (drag_.settled_value()) {
    case SheetValue::expanded:
        if (modal() || !options_.skip_hidden) {
            handle_.set_description(s.dismiss_bottom_sheet);
        } else {
            handle_.set_description(s.collapse_bottom_sheet);
        }
        break;
    case SheetValue::partially_expanded:
        handle_.set_description(s.expand_bottom_sheet);
        break;
    case SheetValue::hidden:
        handle_.set_description(s.expand_bottom_sheet);
        break;
    }
}

void BottomSheet::show(input::InputSource source) {
    if (!drag_.positioned()) {
        return;
    }
    if (drag_.settled_value() == SheetValue::hidden && drag_.idle()) {
        content_.reset_scroll();
        content_.clear_focus();
        own_focus_.clear_focus();
    }
    const SheetValue target = drag_.has(SheetValue::partially_expanded) ? SheetValue::partially_expanded : SheetValue::expanded;
    drag_.animate_to(target, motion_spec().show);
    update_handle();
    if (source == input::InputSource::keyboard && modal()) {
        own_focus_.clear_focus();
        content_.clear_focus();
        tab(false);
    }
}

void BottomSheet::hide() {
    if (drag_.has(SheetValue::hidden)) {
        drag_.animate_to(SheetValue::hidden, motion_spec().hide);
        update_handle();
    }
}

void BottomSheet::expand() {
    drag_.animate_to(SheetValue::expanded, motion_spec().show);
    update_handle();
}

void BottomSheet::partial_expand() {
    if (drag_.has(SheetValue::partially_expanded)) {
        drag_.animate_to(SheetValue::partially_expanded, motion_spec().hide);
        update_handle();
    }
}

void BottomSheet::snap_to(SheetValue value) {
    drag_.snap_to(value);
    scrim_.snap_to(modal() && value != SheetValue::hidden ? 1.0 : 0.0);
    place_parts();
    update_handle();
}

bool BottomSheet::visible() const {
    if (!drag_.positioned()) {
        return false;
    }
    if (!modal() && drag_.has(SheetValue::hidden) == false) {
        return true;
    }
    return drag_.target_value() != SheetValue::hidden || !drag_.idle() || drag_.offset() < area_.height || scrim_.value() > 0.0;
}

bool BottomSheet::is_open() const {
    return visible();
}

void BottomSheet::set_gestures_enabled(bool enabled) {
    options_.gestures = enabled;
    if (!enabled && drag_.pressed()) {
        drag_.cancel_press();
        tracking_ = false;
    }
}

bool BottomSheet::focused() const {
    return own_focus_.has_focus() || content_.focused();
}

void BottomSheet::set_focused(bool focused) {
    if (!focused) {
        own_focus_.clear_focus();
        content_.clear_focus();
        return;
    }
    if (!this->focused()) {
        tab(false);
    }
}

bool BottomSheet::can_focus() const {
    return visible() && (own_focus_.any_can_focus() || content_.can_focus());
}

void BottomSheet::tab(bool backward) {
    std::vector<FocusManager*> parts{&own_focus_};
    if (content_.focus()) {
        parts.push_back(content_.focus());
    }
    chain_focus(parts, backward ? FocusDirection::previous : FocusDirection::next, true);
    content_.reveal_focus();
}

bool BottomSheet::dismiss_after_settle() const {
    return modal() && drag_.target_value() == SheetValue::hidden;
}

BottomSheetResponse BottomSheet::animate_to_dismiss() {
    hide();
    return {.dismissed = modal(), .consumed = true};
}

BottomSheetResponse BottomSheet::settle_to_dismiss() {
    if (drag_.settled_value() == SheetValue::expanded && drag_.has(SheetValue::partially_expanded)) {
        partial_expand();
        return {.consumed = true};
    }
    return animate_to_dismiss();
}

BottomSheetResponse BottomSheet::click_handle() {
    switch (drag_.settled_value()) {
    case SheetValue::expanded:
        if (modal()) {
            return animate_to_dismiss();
        }
        if (!options_.skip_hidden) {
            hide();
        } else {
            partial_expand();
        }
        return {.consumed = true};
    case SheetValue::partially_expanded:
        expand();
        return {.consumed = true};
    case SheetValue::hidden:
        show();
        return {.consumed = true};
    }
    return {.consumed = true};
}

double BottomSheet::damped(double velocity) const {
    if (velocity <= 0.0 || !drag_.has(SheetValue::hidden)) {
        return velocity;
    }
    const double floor = *drag_.position_of(SheetValue::hidden);
    const double distance = std::max(0.0, floor - drag_.offset());
    if (distance >= t().boundary_dampening_zone) {
        return velocity;
    }
    const double factor = distance / t().boundary_dampening_zone;
    double safe = velocity * factor * factor;
    if (velocity >= t().velocity_threshold) {
        safe = std::max(safe, t().velocity_threshold);
    }
    return safe;
}

BottomSheetResponse BottomSheet::finish_drag(double time_seconds) {
    const std::optional<double> v = drag_.release(time_seconds);
    tracking_ = false;
    const double velocity = modal() ? damped(v.value_or(0.0)) : v.value_or(0.0);
    drag_.fling(velocity, motion_spec().settle);
    update_handle();
    return {.dismissed = dismiss_after_settle(), .consumed = true};
}

bool BottomSheet::content_first(const input::PointerEvent& e) const {
    if (e.kind != input::PointerKind::touch || !press_on_content_ || drag_.dragging() || !content_.scrolls()) {
        return false;
    }
    const double dy = e.y_dp - press_y_;
    const ScrollArea& s = content_.scroll();
    if (dy < 0.0) {
        return drag_.offset() <= drag_.min_position() && s.offset() < s.max_offset();
    }
    return dy > 0.0 && s.offset() > 0.0;
}

BottomSheetResponse BottomSheet::handle(const input::PointerEvent& e) {
    if (!visible() || !drag_.positioned()) {
        return {};
    }
    const bool in_area = e.action != input::PointerAction::leave && contains(area_, e.x_dp, e.y_dp);
    const bool in_sheet = in_area && contains(bounds(), e.x_dp, e.y_dp);
    const bool blocks = modal() ? in_area : in_sheet;
    input::PointerEvent clipped = e;
    if (!in_area && e.action == input::PointerAction::move) {
        clipped = left;
    }
    switch (e.action) {
    case input::PointerAction::down: {
        scrim_pressed_ = false;
        content_claimed_ = false;
        tracking_ = false;
        if (in_sheet) {
            if (options_.gestures && (!modal() || drag_.settled_value() != SheetValue::hidden || !drag_.idle())) {
                drag_.press(e.y_dp, e.time_seconds);
                tracking_ = true;
                press_y_ = e.y_dp;
                press_on_content_ = contains(content_.viewport(), e.x_dp, e.y_dp);
            }
            handle_.handle(e);
            content_claimed_ = content_.handle(e);
        } else {
            handle_.handle(left);
            content_.press_elsewhere();
            scrim_pressed_ = modal() && in_area;
        }
        return {.consumed = blocks};
    }
    case input::PointerAction::move: {
        if (tracking_ && !content_claimed_ && !content_first(e)) {
            const bool was_dragging = drag_.dragging();
            if (drag_.move(e.y_dp, e.time_seconds)) {
                if (!was_dragging) {
                    handle_.cancel_interaction();
                    content_.handle(left);
                    content_.cancel_interaction();
                    scrim_pressed_ = false;
                }
                place_parts();
                update_handle();
                return {.consumed = true};
            }
        }
        handle_.handle(clipped);
        const bool used = content_.handle(clipped);
        content_claimed_ = content_claimed_ || (tracking_ && used);
        return {.consumed = blocks};
    }
    case input::PointerAction::up: {
        if (drag_.dragging()) {
            return finish_drag(e.time_seconds);
        }
        drag_.cancel_press();
        tracking_ = false;
        if (scrim_pressed_ && in_area && !in_sheet) {
            scrim_pressed_ = false;
            if (options_.dismiss_on_scrim) {
                return animate_to_dismiss();
            }
            return {.consumed = true};
        }
        scrim_pressed_ = false;
        if (handle_.handle(e)) {
            return click_handle();
        }
        content_.handle(e);
        return {.consumed = blocks};
    }
    case input::PointerAction::leave:
        handle_.handle(e);
        content_.handle(e);
        if (drag_.dragging()) {
            return finish_drag(e.time_seconds);
        }
        drag_.cancel_press();
        tracking_ = false;
        return {};
    }
    return {};
}

BottomSheetResponse BottomSheet::handle(const input::KeyEvent& e) {
    if (!visible()) {
        return {};
    }
    if (modal()) {
        if (e.key == input::Key::escape) {
            return settle_to_dismiss();
        }
        if (e.key == input::Key::tab && !e.ctrl) {
            tab(e.shift);
            return {.consumed = true};
        }
    }
    if (handle_.focused() && handle_.activates(e)) {
        return click_handle();
    }
    const bool consumed = content_.handle(e);
    return {.consumed = modal() || consumed};
}

bool BottomSheet::handle(const input::TextEvent& e) {
    return visible() && content_.handle(e);
}

bool BottomSheet::handle(const input::WheelEvent& e) {
    if (!visible() || !contains(bounds(), e.x_dp, e.y_dp)) {
        return modal() && visible() && contains(area_, e.x_dp, e.y_dp);
    }
    content_.handle(e);
    return true;
}

void BottomSheet::cancel_interaction() {
    handle_.cancel_interaction();
    content_.cancel_interaction();
    if (drag_.dragging()) {
        drag_.cancel_press();
        drag_.fling(0.0, motion_spec().settle);
    }
    drag_.cancel_press();
    tracking_ = false;
    scrim_pressed_ = false;
}

void BottomSheet::step(double dt) {
    drag_.step(dt);
    scrim_.animate_to(modal() && drag_.target_value() != SheetValue::hidden ? 1.0 : 0.0);
    scrim_.step(dt);
    place_parts();
    update_handle();
    handle_.step(dt);
    content_.step(dt);
}

bool BottomSheet::animating() const {
    return drag_.animating() || drag_.pressed() || !scrim_.settled() || handle_.animating() || content_.animating();
}

void BottomSheet::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    if (!visible()) {
        return;
    }
    canvas.save();
    canvas.push_clip(area_);
    if (modal()) {
        paint_scrim(canvas, area_, scheme, scrim_.value());
    }
    const render::Rect s = bounds();
    double overflow = 0.0;
    if (const std::optional<double> top = drag_.position_of(SheetValue::expanded); top && drag_.offset() < *top) {
        overflow = *top - drag_.offset();
    }
    const render::Rect container{s.x, s.y, s.width, s.height + overflow};
    const render::CornerRadii radii = tokens::corner_radii(t().shape, container.width, container.height);
    paint_surface(canvas, container, radii, scheme[t().container], modal() ? t().modal_shadow_elevation : t().standard_shadow_elevation,
                  scheme[color::Role::shadow]);
    canvas.push_clip(render::rounded_rect_path(container, radii));
    if (options_.drag_handle) {
        handle_.paint(canvas, scheme);
    }
    content_.paint(canvas, scheme);
    canvas.pop_clip();
    canvas.pop_clip();
    canvas.restore();
}

std::optional<AccessibleNode> BottomSheet::accessible() const {
    if (!visible()) {
        return std::nullopt;
    }
    AccessibleNode sheet{modal() ? AccessibleRole::dialog : AccessibleRole::group, options_.accessible_name, bounds(), {.modal = modal()}};
    if (options_.drag_handle) {
        sheet.children.push_back(handle_.accessible());
    }
    content_.collect(sheet.children);
    if (modal() && options_.dismiss_on_scrim) {
        sheet.children.push_back({AccessibleRole::button, options_.strings->close_sheet, area_});
    }
    return sheet;
}

}
