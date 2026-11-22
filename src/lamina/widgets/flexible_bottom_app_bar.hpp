#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/floating_toolbar.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/floating_toolbar.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/scroll_area.hpp"

namespace lamina::widgets {

class BottomAppBarState {
public:
    explicit BottomAppBarState(bool exit_always = true);

    bool exit_always() const { return exit_always_; }
    double height_offset() const { return height_offset_; }
    double height_offset_limit() const { return limit_; }
    void set_height_offset_limit(double limit);
    double collapsed_fraction() const { return limit_ != 0.0 ? height_offset_ / limit_ : 0.0; }

    void scrolled_to(double offset);
    bool needs_settle() const;
    void settle();
    bool settling() const { return settling_; }
    void step(double dt_seconds);

private:
    bool exit_always_;
    double limit_ = 0.0;
    double height_offset_ = 0.0;
    double last_ = 0.0;
    bool settling_ = false;
    motion::Animated settle_;
};

enum class BottomBarArrangement { space_between, fixed };

struct FlexibleBottomAppBarOptions {
    std::vector<ToolbarAction> actions;
    BottomBarArrangement arrangement = BottomBarArrangement::space_between;
    bool exit_always = true;
    std::string accessible_name = {};
};

class FlexibleBottomAppBar {
public:
    explicit FlexibleBottomAppBar(FlexibleBottomAppBarOptions options);

    double width() const { return width_; }
    double height() const;
    void set_width(double width);
    void set_origin(render::Point origin);
    void set_bottom(double bottom);
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height()}; }
    BottomAppBarState& state() { return state_; }
    const BottomAppBarState& state() const { return state_; }
    void follow(const ScrollArea& scroll);

    std::size_t size() const { return actions_.size(); }
    IconButton& action(std::size_t index) { return *actions_[index]; }
    const IconButton& action(std::size_t index) const { return *actions_[index]; }
    std::vector<FocusStop*> focus_stops();

    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    std::optional<std::size_t> handle(const input::PointerEvent& event);
    std::optional<std::size_t> handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    template <typename Offer>
    void offer_targets(const input::PointerEvent& event, Offer offer) const;
    void place();
    bool shown() const { return height() > 0.0; }

    FlexibleBottomAppBarOptions options_;
    std::vector<std::unique_ptr<IconButton>> actions_;
    TargetArbiter arbiter_;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    BottomAppBarState state_;
    double width_ = 0.0;
    std::optional<double> bottom_;
    render::Point origin_{0.0, 0.0};
    bool enabled_ = true;
};

}
