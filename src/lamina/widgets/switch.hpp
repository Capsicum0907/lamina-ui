#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/switch.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct SwitchOptions {
    bool checked = false;
    std::optional<icons::Icon> checked_icon = std::nullopt;
    std::optional<icons::Icon> unchecked_icon = std::nullopt;
    bool enabled = true;
    std::string accessible_name = {};
};

class Switch {
public:
    explicit Switch(SwitchOptions options = {});

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool checked() const { return options_.checked; }
    void set_checked(bool checked);
    double thumb_size() const { return thumb_size_.value(); }
    double thumb_center_x() const;

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const {
        return {AccessibleRole::switch_control, options_.accessible_name, bounds(),
                {.enabled = enabled(), .focusable = enabled(), .focused = focused(),
                 .toggle = options_.checked ? AccessibleToggle::on : AccessibleToggle::off}};
    }

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    const std::optional<icons::Icon>& current_icon() const;
    double target_thumb_size() const;
    void retarget();

    SwitchOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    motion::Animated offset_;
    motion::Animated thumb_size_;
    motion::Animated state_layer_;
    motion::Animated checked_;
    StyleStamp style_stamp_;
};

}
