#include "lamina/widgets/scroll_area.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::ScrollAreaTokens& t = tokens::scroll_area_tokens;

}

ScrollArea::ScrollArea()
    : offset_(0.0, t.offset_spring),
      offset_x_(0.0, t.offset_spring),
      visibility_(0.0, t.bar_spring),
      hover_(0.0, t.bar_spring),
      hover_x_(0.0, t.bar_spring),
      fling_(fling_decay()),
      fling_x_(fling_decay()) {}

void ScrollArea::set_viewport(const render::Rect& viewport) {
    viewport_ = viewport;
    retarget(offset_.target());
    retarget_x(offset_x_.target());
}

void ScrollArea::set_content_height(double height) {
    content_height_ = std::max(0.0, height);
    retarget(offset_.target());
}

void ScrollArea::set_content_width(double width) {
    content_width_ = std::max(0.0, width);
    retarget_x(offset_x_.target());
}

double ScrollArea::max_offset() const {
    return std::max(0.0, content_height_ - viewport_.height);
}

double ScrollArea::max_offset_x() const {
    return std::max(0.0, content_width_ - viewport_.width);
}

void ScrollArea::retarget(double offset) {
    const double clamped = std::clamp(offset, 0.0, max_offset());
    if (clamped != offset_.target()) {
        offset_.animate_to(clamped);
    }
}

void ScrollArea::retarget_x(double offset) {
    const double clamped = std::clamp(offset, 0.0, max_offset_x());
    if (clamped != offset_x_.target()) {
        offset_x_.animate_to(clamped);
    }
}

void ScrollArea::reveal() {
    if (!scrollable() && !scrollable_x()) {
        return;
    }
    visibility_.animate_to(1.0);
    hide_in_ = t.hide_delay_seconds;
}

void ScrollArea::scroll_by(double dy) {
    scroll_to(offset_.target() + dy);
}

void ScrollArea::scroll_to(double offset) {
    fling_.stop();
    const double before = offset_.target();
    retarget(offset);
    if (offset_.target() != before) {
        reveal();
    }
}

void ScrollArea::jump_to(double offset) {
    fling_.stop();
    offset_.snap_to(std::clamp(offset, 0.0, max_offset()));
}

void ScrollArea::ensure_visible(double top, double bottom) {
    const double target = offset_.target();
    const double height = viewport_.height;
    if (top < target || bottom - top >= height) {
        scroll_to(top);
    } else if (bottom > target + height) {
        scroll_to(bottom - height);
    }
}

void ScrollArea::scroll_by_x(double dx) {
    scroll_to_x(offset_x_.target() + dx);
}

void ScrollArea::scroll_to_x(double offset) {
    fling_x_.stop();
    const double before = offset_x_.target();
    retarget_x(offset);
    if (offset_x_.target() != before) {
        reveal();
    }
}

void ScrollArea::jump_to_x(double offset) {
    fling_x_.stop();
    offset_x_.snap_to(std::clamp(offset, 0.0, max_offset_x()));
}

void ScrollArea::ensure_visible_x(double left, double right) {
    const double target = offset_x_.target();
    const double width = viewport_.width;
    if (left < target || right - left >= width) {
        scroll_to_x(left);
    } else if (right > target + width) {
        scroll_to_x(right - width);
    }
}

bool ScrollArea::over_bar(double x, double y) const {
    const render::Rect zone{viewport_.x + viewport_.width - t.bar_hit_width, viewport_.y, t.bar_hit_width, viewport_.height};
    return scrollable() && contains(zone, x, y);
}

bool ScrollArea::over_bar_x(double x, double y) const {
    const double end = scrollable() ? t.bar_hit_width : 0.0;
    const render::Rect zone{viewport_.x, viewport_.y + viewport_.height - t.bar_hit_width, viewport_.width - end, t.bar_hit_width};
    return scrollable_x() && contains(zone, x, y);
}

double ScrollArea::track_length() const {
    const double corner = scrollable_x() ? t.bar_hit_width : 0.0;
    return std::max(0.0, viewport_.height - 2.0 * t.bar_margin - corner);
}

double ScrollArea::thumb_length() const {
    const double track = track_length();
    if (content_height_ <= 0.0) {
        return track;
    }
    return std::min(track, std::max(t.min_thumb_length, track * viewport_.height / content_height_));
}

double ScrollArea::track_length_x() const {
    const double corner = scrollable() ? t.bar_hit_width : 0.0;
    return std::max(0.0, viewport_.width - 2.0 * t.bar_margin - corner);
}

double ScrollArea::thumb_length_x() const {
    const double track = track_length_x();
    if (content_width_ <= 0.0) {
        return track;
    }
    return std::min(track, std::max(t.min_thumb_length, track * viewport_.width / content_width_));
}

bool ScrollArea::handle(const input::WheelEvent& e) {
    if (!contains(viewport_, e.x_dp, e.y_dp) || (!scrollable() && !scrollable_x())) {
        return false;
    }
    const bool sideways = e.shift && e.dx == 0.0 && scrollable_x();
    const double dx = sideways ? e.dy : e.dx;
    const double dy = sideways ? 0.0 : e.dy;
    if (dy != 0.0 && scrollable()) {
        scroll_by(dy * t.wheel_step);
    }
    if (dx != 0.0 && scrollable_x()) {
        scroll_by_x(dx * t.wheel_step);
    }
    return scrollable() || (dx != 0.0 && scrollable_x());
}

bool ScrollArea::press_bar_x(const input::PointerEvent& e) {
    const std::optional<render::Rect> r = thumb_x();
    if (r && e.x_dp >= r->x && e.x_dp < r->x + r->width) {
        dragging_ = true;
        drag_ = Drag::horizontal;
        drag_start_y_ = e.x_dp;
        drag_start_offset_ = offset_x_.value();
        offset_x_.snap_to(offset_x_.value());
    } else if (r) {
        const double page = std::max(viewport_.width - t.page_overlap, t.wheel_step);
        scroll_by_x(e.x_dp < r->x ? -page : page);
    }
    return true;
}

bool ScrollArea::handle(const input::PointerEvent& e) {
    if (e.kind == input::PointerKind::touch && e.action != input::PointerAction::leave && !dragging_) {
        pointer_.reset();
        bar_hovered_ = false;
        bar_x_hovered_ = false;
        hover_.animate_to(0.0);
        hover_x_.animate_to(0.0);
        return false;
    }
    if (e.action == input::PointerAction::leave) {
        pointer_.reset();
        bar_hovered_ = false;
        bar_x_hovered_ = false;
        dragging_ = false;
        drag_ = Drag::none;
        hover_.animate_to(0.0);
        hover_x_.animate_to(0.0);
        return false;
    }
    pointer_ = render::Point{e.x_dp, e.y_dp};
    hovered_offset_ = offset_.value();
    hovered_offset_x_ = offset_x_.value();
    if (dragging_) {
        if (e.action == input::PointerAction::up) {
            dragging_ = false;
            drag_ = Drag::none;
            bar_hovered_ = over_bar(e.x_dp, e.y_dp);
            bar_x_hovered_ = !bar_hovered_ && over_bar_x(e.x_dp, e.y_dp);
            hover_.animate_to(bar_hovered_ ? 1.0 : 0.0);
            hover_x_.animate_to(bar_x_hovered_ ? 1.0 : 0.0);
            hide_in_ = t.hide_delay_seconds;
            return true;
        }
        if (drag_ == Drag::horizontal) {
            const double travel = track_length_x() - thumb_length_x();
            if (travel > 0.0) {
                jump_to_x(drag_start_offset_ + (e.x_dp - drag_start_y_) * max_offset_x() / travel);
            }
            return true;
        }
        const double travel = track_length() - thumb_length();
        if (travel > 0.0) {
            jump_to(drag_start_offset_ + (e.y_dp - drag_start_y_) * max_offset() / travel);
        }
        return true;
    }
    bar_hovered_ = over_bar(e.x_dp, e.y_dp);
    bar_x_hovered_ = !bar_hovered_ && over_bar_x(e.x_dp, e.y_dp);
    hover_.animate_to(bar_hovered_ ? 1.0 : 0.0);
    hover_x_.animate_to(bar_x_hovered_ ? 1.0 : 0.0);
    if (bar_hovered_ || bar_x_hovered_) {
        reveal();
    }
    if (e.action != input::PointerAction::down || (!bar_hovered_ && !bar_x_hovered_)) {
        return false;
    }
    if (bar_x_hovered_) {
        return press_bar_x(e);
    }
    const std::optional<render::Rect> r = thumb();
    if (r && e.y_dp >= r->y && e.y_dp < r->y + r->height) {
        dragging_ = true;
        drag_ = Drag::vertical;
        drag_start_y_ = e.y_dp;
        drag_start_offset_ = offset_.value();
        offset_.snap_to(offset_.value());
    } else if (r) {
        const double page = std::max(viewport_.height - t.page_overlap, t.wheel_step);
        scroll_by(e.y_dp < r->y ? -page : page);
    }
    return true;
}

bool ScrollArea::handle(const input::KeyEvent& e) {
    const double page = std::max(viewport_.height - t.page_overlap, t.wheel_step);
    if (e.key == input::Key::page_down) {
        scroll_by(page);
        return true;
    }
    if (e.key == input::Key::page_up) {
        scroll_by(-page);
        return true;
    }
    return false;
}

void ScrollArea::stop_fling() {
    fling_.stop();
    fling_x_.stop();
}

input::ScrollAxes ScrollArea::touch_axes() const {
    return {scrollable_x(), scrollable()};
}

void ScrollArea::follow_y(double offset) {
    offset_.snap_to(std::clamp(offset, 0.0, max_offset()));
}

void ScrollArea::follow_x(double offset) {
    offset_x_.snap_to(std::clamp(offset, 0.0, max_offset_x()));
}

void ScrollArea::follow_touch(const TouchScrollUpdate& u) {
    if (u.dy != 0.0) {
        follow_y(offset_.value() - u.dy);
    }
    if (u.dx != 0.0) {
        follow_x(offset_x_.value() - u.dx);
    }
    reveal();
}

ScrollArea::TouchRoute ScrollArea::touch_begin(const input::PointerEvent& e) {
    pointer_.reset();
    switch (e.action) {
    case input::PointerAction::down: {
        const bool inside = contains(viewport_, e.x_dp, e.y_dp);
        const input::ScrollAxes axes = inside ? touch_axes() : input::ScrollAxes{};
        const bool caught = axes.any() && flinging();
        if (caught) {
            stop_fling();
        }
        if (touch_.press(e, axes, caught).step == TouchScrollStep::caught) {
            return TouchRoute::claimed;
        }
        return TouchRoute::deliver_then_check;
    }
    case input::PointerAction::move:
        if (touch_.owns()) {
            follow_touch(touch_.move(e));
            return TouchRoute::consumed;
        }
        return touch_.pending() ? TouchRoute::deliver_then_check : TouchRoute::deliver;
    case input::PointerAction::up:
        if (touch_.owns()) {
            const TouchScrollUpdate u = touch_.release(e);
            follow_touch(u);
            const double floor = touch_.tokens().minimum_fling_velocity;
            if (std::abs(u.velocity_y) > floor) {
                fling_.start(offset_.value(), -u.velocity_y);
            }
            if (std::abs(u.velocity_x) > floor) {
                fling_x_.start(offset_x_.value(), -u.velocity_x);
            }
            hide_in_ = t.hide_delay_seconds;
            return TouchRoute::consumed;
        }
        touch_.cancel();
        return TouchRoute::deliver;
    case input::PointerAction::leave:
        break;
    }
    touch_.cancel();
    return TouchRoute::deliver;
}

bool ScrollArea::touch_after_children(const input::PointerEvent& e) {
    if (e.action != input::PointerAction::move || !touch_.pending()) {
        return false;
    }
    const TouchScrollUpdate u = touch_.move(e);
    if (u.step != TouchScrollStep::claim) {
        return false;
    }
    stop_fling();
    follow_touch(u);
    return true;
}

void ScrollArea::cancel_interaction() {
    touch_.cancel();
    stop_fling();
    dragging_ = false;
    drag_ = Drag::none;
    bar_hovered_ = false;
    bar_x_hovered_ = false;
    hover_.animate_to(0.0);
    hover_x_.animate_to(0.0);
}

input::PointerEvent ScrollArea::to_content(const input::PointerEvent& e) const {
    if (e.action == input::PointerAction::leave || dragging() || !contains(viewport_, e.x_dp, e.y_dp) || over_bar(e.x_dp, e.y_dp) ||
        over_bar_x(e.x_dp, e.y_dp)) {
        return {input::PointerAction::leave, 0.0, 0.0};
    }
    input::PointerEvent inside = e;
    inside.x_dp = e.x_dp - viewport_.x + offset_x_.value();
    inside.y_dp = e.y_dp - viewport_.y + offset_.value();
    return inside;
}

render::Rect ScrollArea::to_window(const render::Rect& r) const {
    return {r.x + viewport_.x - offset_x_.value(), r.y + viewport_.y - offset_.value(), r.width, r.height};
}

std::optional<input::PointerEvent> ScrollArea::rehover() {
    if (!pointer_ || (offset_.value() == hovered_offset_ && offset_x_.value() == hovered_offset_x_)) {
        return std::nullopt;
    }
    hovered_offset_ = offset_.value();
    hovered_offset_x_ = offset_x_.value();
    return to_content({input::PointerAction::move, pointer_->x, pointer_->y});
}

void ScrollArea::step(double dt) {
    const double edge = touch_.tokens().edge_tolerance;
    if (fling_.active()) {
        const double v = fling_.step(dt);
        follow_y(v);
        if (std::abs(v - offset_.value()) > edge) {
            fling_.stop();
        }
        reveal();
    }
    if (fling_x_.active()) {
        const double v = fling_x_.step(dt);
        follow_x(v);
        if (std::abs(v - offset_x_.value()) > edge) {
            fling_x_.stop();
        }
        reveal();
    }
    offset_.step(dt);
    offset_x_.step(dt);
    visibility_.step(dt);
    hover_.step(dt);
    hover_x_.step(dt);
    if (!offset_.settled() || !offset_x_.settled()) {
        reveal();
    }
    const bool hovered = bar_hovered_ || bar_x_hovered_;
    if (!scrollable() && !scrollable_x()) {
        hide_in_ = -1.0;
        visibility_.animate_to(0.0);
    } else if (hide_in_ >= 0.0 && !hovered && !dragging() && !flinging()) {
        hide_in_ -= dt;
        if (hide_in_ < 0.0) {
            visibility_.animate_to(0.0);
        }
    }
}

bool ScrollArea::animating() const {
    return flinging() || !offset_.settled() || !offset_x_.settled() || !visibility_.settled() || !hover_.settled() || !hover_x_.settled();
}

double ScrollArea::next_wake_seconds() const {
    if (hide_in_ < 0.0 || bar_hovered_ || bar_x_hovered_ || dragging() || flinging()) {
        return -1.0;
    }
    return hide_in_;
}

std::optional<render::Rect> ScrollArea::thumb() const {
    if (!scrollable()) {
        return std::nullopt;
    }
    const double thickness = motion::lerp(t.bar_thickness, t.hovered_bar_thickness, std::clamp(hover_.value(), 0.0, 1.0));
    const double length = thumb_length();
    const double progress = std::clamp(offset_.value() / max_offset(), 0.0, 1.0);
    return render::Rect{viewport_.x + viewport_.width - t.bar_margin - thickness,
                        viewport_.y + t.bar_margin + (track_length() - length) * progress, thickness, length};
}

std::optional<render::Rect> ScrollArea::thumb_x() const {
    if (!scrollable_x()) {
        return std::nullopt;
    }
    const double thickness = motion::lerp(t.bar_thickness, t.hovered_bar_thickness, std::clamp(hover_x_.value(), 0.0, 1.0));
    const double length = thumb_length_x();
    const double progress = std::clamp(offset_x_.value() / max_offset_x(), 0.0, 1.0);
    return render::Rect{viewport_.x + t.bar_margin + (track_length_x() - length) * progress,
                        viewport_.y + viewport_.height - t.bar_margin - thickness, length, thickness};
}

void ScrollArea::begin_content(render::Canvas& canvas) const {
    canvas.save();
    canvas.push_clip(viewport_);
    canvas.translate(viewport_.x - offset_x_.value(), viewport_.y - offset_.value());
}

void ScrollArea::end_content(render::Canvas& canvas) const {
    canvas.pop_clip();
    canvas.restore();
}

void ScrollArea::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double shown = std::clamp(visibility_.value(), 0.0, 1.0);
    if (const std::optional<render::Rect> r = thumb()) {
        const double opacity = motion::lerp(t.bar_opacity, t.hovered_bar_opacity, std::clamp(hover_.value(), 0.0, 1.0)) * shown;
        if (opacity > 0.0) {
            canvas.fill_rounded_rect(*r, render::uniform_radii(r->width / 2.0), color::with_alpha(scheme[t.bar], opacity));
        }
    }
    if (const std::optional<render::Rect> r = thumb_x()) {
        const double opacity = motion::lerp(t.bar_opacity, t.hovered_bar_opacity, std::clamp(hover_x_.value(), 0.0, 1.0)) * shown;
        if (opacity > 0.0) {
            canvas.fill_rounded_rect(*r, render::uniform_radii(r->height / 2.0), color::with_alpha(scheme[t.bar], opacity));
        }
    }
}

}
