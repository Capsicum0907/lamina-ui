#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/badge.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct IconButtonOptions {
    icons::Icon icon;
    std::string accessible_name;
    tokens::IconButtonStyle style = tokens::IconButtonStyle::standard;
    tokens::IconButtonSize size = tokens::IconButtonSize::small;
    tokens::ButtonShape shape = tokens::ButtonShape::round;
    tokens::IconButtonWidth width = tokens::IconButtonWidth::uniform;
    bool toggleable = false;
    bool checked = false;
    bool enabled = true;
};

class IconButton {
public:
    explicit IconButton(IconButtonOptions options);

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool checked() const { return options_.checked; }
    void set_checked(bool checked);
    void set_icon(icons::Icon icon);
    void set_badge(std::optional<Badge> badge) { badge_ = std::move(badge); }
    const std::optional<Badge>& badge() const { return badge_; }
    render::Rect icon_rect() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    const std::string& accessible_name() const { return options_.accessible_name; }
    void set_accessible_name(std::string name) { options_.accessible_name = required_name(std::move(name), "An icon button"); }
    AccessibleNode accessible() const {
        AccessibleState state{.enabled = enabled(), .focusable = enabled(), .focused = focused()};
        if (options_.toggleable) {
            state.toggle = options_.checked ? AccessibleToggle::on : AccessibleToggle::off;
        }
        return {options_.toggleable ? AccessibleRole::toggle_button : AccessibleRole::button, options_.accessible_name, bounds(), state, {},
                badge_ ? badge_->description() : std::string()};
    }

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double target_corner() const;
    void retarget();

    IconButtonOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    motion::Animated corner_;
    motion::Animated state_layer_;
    motion::Animated checked_;
    std::optional<icons::Icon> previous_icon_;
    std::optional<Badge> badge_;
    motion::Animated icon_fade_;
};

}
