#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/paging.hpp"
#include "lamina/motion/spring.hpp"
#include "lamina/motion/velocity_tracker.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/pager.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"

namespace lamina::widgets {

struct PagerOptions {
    std::size_t count = 0;
    std::size_t page = 0;
    std::string accessible_name = {};
    bool mouse_drag = true;
    const tokens::PagerTokens* tokens = &current_style().pager;
};

class Pager {
public:
    explicit Pager(PagerOptions options);

    void set_bounds(const render::Rect& bounds) { bounds_ = bounds; }
    const render::Rect& bounds() const { return bounds_; }
    std::size_t count() const { return count_; }
    void set_count(std::size_t count);
    const tokens::PagerTokens& tokens() const { return *tokens_; }

    double position() const { return position_; }
    std::size_t current_page() const { return motion::nearest_page(position_, count_); }
    double current_page_offset_fraction() const { return position_ - static_cast<double>(current_page()); }
    std::size_t settled_page() const { return settled_; }
    std::size_t target_page() const { return settling_ ? target_ : current_page(); }
    double page_pitch() const { return bounds_.width + tokens_->page_spacing; }
    double page_offset(std::size_t page) const { return (static_cast<double>(page) - position_) * page_pitch(); }
    render::Rect page_rect(std::size_t page) const { return {bounds_.x + page_offset(page), bounds_.y, bounds_.width, bounds_.height}; }
    motion::PageRange pages_in_view() const { return motion::pages_in_view(position_, count_); }
    motion::PageRange pages_to_build() const { return motion::pages_around(position_, count_, tokens_->beyond_viewport_pages); }

    bool dragging() const { return gesture_ == Gesture::dragging; }
    bool settling() const { return settling_; }
    bool scroll_in_progress() const { return dragging() || settling(); }
    bool swiping() const { return dragging() || (settling_ && source_ == Source::swipe); }
    bool holds_pointer() const { return dragging(); }
    bool can_move(double finger_dx) const;

    void scroll_to_page(std::size_t page);
    void animate_to_page(std::size_t page);

    template <typename Deliver>
    bool route(const input::PointerEvent& event, Deliver&& deliver);

    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const { return settling_ || !focus_.settled(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused) { focus_.set_focused(focused && can_focus()); }
    bool can_focus() const { return enabled_ && count_ > 1; }
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    void begin_page(render::Canvas& canvas, std::size_t page) const;
    void end_page(render::Canvas& canvas) const;
    void paint_focus(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    enum class Gesture { idle, pending, dragging };
    enum class Source { swipe, request };
    enum class Route { deliver, deliver_then_check, consumed, claimed };

    template <typename Deliver>
    static bool deliver_holds(Deliver& deliver, const input::PointerEvent& e) {
        if constexpr (std::is_void_v<std::invoke_result_t<Deliver&, const input::PointerEvent&>>) {
            deliver(e);
            return false;
        } else {
            return static_cast<bool>(deliver(e));
        }
    }

    bool accepts(const input::PointerEvent& e) const;
    Route begin(const input::PointerEvent& e);
    bool after_child(const input::PointerEvent& e, bool holds);
    input::PointerEvent forward(const input::PointerEvent& e) const;
    void follow(double finger_dx);
    void release(const input::PointerEvent& e);
    void settle_to(std::size_t page, double velocity, Source source);
    void finish_settle();

    const tokens::PagerTokens* tokens_;
    std::string name_;
    bool mouse_drag_;
    std::size_t count_;
    render::Rect bounds_{0.0, 0.0, 0.0, 0.0};
    bool enabled_ = true;
    FocusIndicator focus_;

    double position_ = 0.0;
    std::size_t settled_ = 0;
    bool settling_ = false;
    std::size_t target_ = 0;
    double settle_from_ = 0.0;
    motion::SpringState spring_{0.0, 0.0};
    Source source_ = Source::swipe;

    Gesture gesture_ = Gesture::idle;
    double press_x_ = 0.0;
    double origin_x_ = 0.0;
    double origin_y_ = 0.0;
    double last_x_ = 0.0;
    motion::VelocityTracker tracker_;
};

template <typename Deliver>
bool Pager::route(const input::PointerEvent& e, Deliver&& deliver) {
    const input::PointerEvent leave{input::PointerAction::leave, 0.0, 0.0};
    switch (begin(e)) {
    case Route::consumed:
        return true;
    case Route::claimed:
        deliver_holds(deliver, leave);
        return true;
    case Route::deliver_then_check:
        if (after_child(e, deliver_holds(deliver, forward(e)))) {
            deliver_holds(deliver, leave);
            return true;
        }
        return false;
    case Route::deliver:
        break;
    }
    deliver_holds(deliver, e);
    return false;
}

}
