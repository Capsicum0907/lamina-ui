#pragma once

#include <optional>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/content_slot.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::widgets {

class ContentHost {
public:
    explicit ContentHost(std::optional<ContentSlot> slot);

    bool empty() const { return !slot_ || !slot_->node; }
    double measure(double width);
    void place(const render::Rect& viewport);
    const render::Rect& viewport() const { return scroll_.viewport(); }
    double content_height() const { return height_; }
    bool scrolls() const { return scroll_.max_offset() > 0.0; }
    const ScrollArea& scroll() const { return scroll_; }
    void reset_scroll() { scroll_.jump_to(0.0); }
    void relayout() { measured_width_ = -1.0; }

    FocusManager* focus() const { return slot_ ? slot_->focus : nullptr; }
    bool focused() const { return focus() && focus()->has_focus(); }
    bool can_focus() const { return focus() && focus()->any_can_focus(); }
    void clear_focus();
    void reveal_focus();

    bool handle(const input::PointerEvent& event);
    bool holds_pointer() const;
    bool handle(const input::WheelEvent& event);
    bool handle(const input::KeyEvent& event);
    bool handle(const input::TextEvent& event);
    void press_elsewhere();
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void collect(std::vector<AccessibleNode>& out) const;
    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;

private:
    WidgetSet* widgets() const { return slot_ ? slot_->widgets : nullptr; }

    std::optional<ContentSlot> slot_;
    ScrollArea scroll_;
    double height_ = 0.0;
    double measured_width_ = -1.0;
    double arranged_width_ = -1.0;
};

}
