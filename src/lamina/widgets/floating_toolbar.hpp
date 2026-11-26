#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/floating_toolbar.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

struct ToolbarAction {
    icons::Icon icon;
    std::string accessible_name;
    bool enabled = true;
};

struct ToolbarFab {
    icons::Icon icon;
    std::string accessible_name;
};

enum class ToolbarOrientation { horizontal, vertical };
enum class ToolbarColorStyle { standard, vibrant };
enum class ToolbarFabPosition { start, end };
enum class ToolbarSlot { leading, content, trailing };

struct FloatingToolbarOptions {
    std::vector<ToolbarAction> actions;
    std::vector<ToolbarAction> leading = {};
    std::vector<ToolbarAction> trailing = {};
    std::optional<ToolbarFab> fab = std::nullopt;
    ToolbarOrientation orientation = ToolbarOrientation::horizontal;
    ToolbarColorStyle colors = ToolbarColorStyle::standard;
    ToolbarFabPosition fab_position = ToolbarFabPosition::end;
    bool expanded = true;
    std::string accessible_name = {};
    const PatternStrings* strings = &english_us_patterns();
};

struct ToolbarClick {
    ToolbarSlot slot;
    std::size_t index;
};

struct FloatingToolbarResponse {
    std::optional<ToolbarClick> clicked = std::nullopt;
    bool fab_clicked = false;
    bool consumed = false;
};

class ToolbarScrollExpansion {
public:
    explicit ToolbarScrollExpansion(bool expanded = true, double threshold = current_style().floating_toolbar.scroll_threshold);

    std::optional<bool> scrolled_to(double offset);
    void set_expanded(bool expanded);
    bool expanded() const { return expanded_; }

private:
    void update_threshold();

    bool expanded_;
    double threshold_distance_;
    double content_offset_ = 0.0;
    double threshold_ = 0.0;
    double last_ = 0.0;
};

class FloatingToolbar {
public:
    FloatingToolbar(FloatingToolbarOptions options, const icons::IconFont& icon_font);

    double width() const;
    double height() const;
    void set_origin(render::Point origin);
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }
    render::Rect toolbar_rect() const;
    std::optional<render::Rect> fab_rect() const;

    bool expanded() const { return options_.expanded; }
    void set_expanded(bool expanded);
    double expanded_progress() const { return progress_.value(); }
    bool has_fab() const { return options_.fab.has_value(); }
    ToolbarOrientation orientation() const { return options_.orientation; }

    std::size_t action_count(ToolbarSlot slot) const;
    IconButton& action(ToolbarSlot slot, std::size_t index);
    const IconButton& action(ToolbarSlot slot, std::size_t index) const;
    bool fab_focused() const { return fab_focus_.focused(); }
    void set_fab_focused(bool focused);
    std::vector<FocusStop*> focus_stops();

    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    FloatingToolbarResponse handle(const input::PointerEvent& event);
    FloatingToolbarResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Group {
        std::vector<std::unique_ptr<IconButton>> buttons;
        motion::Animated shown;
        bool expanding = true;
    };

    bool horizontal() const { return options_.orientation == ToolbarOrientation::horizontal; }
    const tokens::ToolbarColorTokens& colors() const;
    template <typename Offer>
    void offer_targets(const input::PointerEvent& event, Offer offer) const;
    Group& group(ToolbarSlot slot);
    const Group& group(ToolbarSlot slot) const;
    double extent(const Group& g) const;
    double full_main() const;
    double fab_size() const;
    bool shows(ToolbarSlot slot, std::size_t index) const;
    void place();
    double fab_target_elevation() const;
    void paint_group(render::Canvas& canvas, const color::ColorScheme& scheme, const Group& g) const;

    FloatingToolbarOptions options_;
    Group leading_;
    Group content_;
    Group trailing_;
    motion::Animated progress_;
    motion::Animated fab_state_layer_;
    motion::Animated fab_elevation_;
    PressState fab_press_;
    FocusIndicator fab_focus_;
    Enablement enabled_;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    render::Point origin_{0.0, 0.0};
    TargetArbiter arbiter_;
    StyleStamp style_stamp_;
};

}
