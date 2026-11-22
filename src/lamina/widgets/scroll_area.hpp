#pragma once

#include <optional>
#include <type_traits>
#include <utility>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/spline_decay.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/scroll_area.hpp"
#include "lamina/widgets/touch_scroll.hpp"

namespace lamina::widgets {

class ScrollArea {
public:
    ScrollArea();

    void set_viewport(const render::Rect& viewport);
    void set_content_height(double height);
    void set_content_width(double width);
    const render::Rect& viewport() const { return viewport_; }
    double content_height() const { return content_height_; }
    double content_width() const { return content_width_; }
    double max_offset() const;
    double max_offset_x() const;
    double offset() const { return offset_.value(); }
    double target_offset() const { return offset_.target(); }
    double offset_x() const { return offset_x_.value(); }
    double target_offset_x() const { return offset_x_.target(); }

    void scroll_by(double dy);
    void scroll_to(double offset);
    void jump_to(double offset);
    void ensure_visible(double top, double bottom);
    void scroll_by_x(double dx);
    void scroll_to_x(double offset);
    void jump_to_x(double offset);
    void ensure_visible_x(double left, double right);

    bool handle(const input::WheelEvent& event);
    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();

    template <typename Deliver>
    bool route(const input::PointerEvent& event, Deliver&& deliver);

    input::PointerEvent to_content(const input::PointerEvent& event) const;
    render::Rect to_window(const render::Rect& content) const;
    std::optional<input::PointerEvent> rehover();

    void step(double dt_seconds);
    bool animating() const;
    bool dragging() const { return dragging_ || touch_.owns(); }
    bool flinging() const { return fling_.active() || fling_x_.active(); }
    bool scroll_in_progress() const { return dragging() || flinging(); }
    void stop_fling();
    double next_wake_seconds() const;
    double bar_visibility() const { return visibility_.value(); }
    std::optional<render::Rect> thumb() const;
    std::optional<render::Rect> thumb_x() const;

    void begin_content(render::Canvas& canvas) const;
    void end_content(render::Canvas& canvas) const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    enum class Drag { none, vertical, horizontal };
    enum class TouchRoute { deliver, deliver_then_check, consumed, claimed };

    template <typename Deliver>
    static bool deliver_holds(Deliver& deliver, const input::PointerEvent& e) {
        if constexpr (std::is_void_v<std::invoke_result_t<Deliver&, const input::PointerEvent&>>) {
            deliver(e);
            return false;
        } else {
            return static_cast<bool>(deliver(e));
        }
    }

    input::ScrollAxes touch_axes() const;
    TouchRoute touch_begin(const input::PointerEvent& e);
    bool touch_after_children(const input::PointerEvent& e);
    void follow_touch(const TouchScrollUpdate& u);
    void follow_y(double offset);
    void follow_x(double offset);

    bool scrollable() const { return max_offset() > 0.0; }
    bool scrollable_x() const { return max_offset_x() > 0.0; }
    bool over_bar(double x, double y) const;
    bool over_bar_x(double x, double y) const;
    double track_length() const;
    double thumb_length() const;
    double track_length_x() const;
    double thumb_length_x() const;
    void reveal();
    void retarget(double offset);
    void retarget_x(double offset);
    bool press_bar_x(const input::PointerEvent& e);

    render::Rect viewport_{0.0, 0.0, 0.0, 0.0};
    double content_height_ = 0.0;
    double content_width_ = 0.0;
    motion::Animated offset_;
    motion::Animated offset_x_;
    motion::Animated visibility_;
    motion::Animated hover_;
    motion::Animated hover_x_;
    double hide_in_ = -1.0;
    bool bar_hovered_ = false;
    bool bar_x_hovered_ = false;
    bool dragging_ = false;
    Drag drag_ = Drag::none;
    double drag_start_y_ = 0.0;
    double drag_start_offset_ = 0.0;
    std::optional<render::Point> pointer_;
    double hovered_offset_ = 0.0;
    double hovered_offset_x_ = 0.0;
    TouchScroll touch_;
    motion::Fling fling_;
    motion::Fling fling_x_;
};

template <typename Deliver>
bool ScrollArea::route(const input::PointerEvent& e, Deliver&& deliver) {
    const input::PointerEvent leave{input::PointerAction::leave, 0.0, 0.0};
    if (e.kind != input::PointerKind::touch || e.action == input::PointerAction::leave) {
        if (e.action == input::PointerAction::leave) {
            touch_.cancel();
        }
        if (handle(e)) {
            deliver_holds(deliver, leave);
            return true;
        }
        deliver_holds(deliver, e);
        return false;
    }
    switch (touch_begin(e)) {
    case TouchRoute::consumed:
        return true;
    case TouchRoute::claimed:
        deliver_holds(deliver, leave);
        return true;
    case TouchRoute::deliver_then_check:
        if (deliver_holds(deliver, touch_.forward(e))) {
            touch_.yield();
            return false;
        }
        if (touch_after_children(e)) {
            deliver_holds(deliver, leave);
            return true;
        }
        return false;
    case TouchRoute::deliver:
        break;
    }
    deliver_holds(deliver, e);
    return false;
}

}
