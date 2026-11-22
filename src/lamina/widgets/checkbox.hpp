#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include <optional>
#include <string>

#include "lamina/tokens/checkbox.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct CheckboxOptions {
    tokens::CheckboxState state = tokens::CheckboxState::off;
    bool enabled = true;
    std::string accessible_name = {};
};

render::Path check_mark_path(const render::Rect& box, double draw_fraction, double center_gravitation, double stroke_width);

class Checkbox {
public:
    explicit Checkbox(CheckboxOptions options = {});

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    render::Rect box() const;

    tokens::CheckboxState state() const { return options_.state; }
    void set_state(tokens::CheckboxState state);
    double check_fraction() const { return check_fraction_.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const {
        const AccessibleToggle toggle = options_.state == tokens::CheckboxState::on              ? AccessibleToggle::on
                                        : options_.state == tokens::CheckboxState::indeterminate ? AccessibleToggle::mixed
                                                                                                 : AccessibleToggle::off;
        return {AccessibleRole::checkbox, options_.accessible_name, bounds(),
                {.enabled = enabled(), .focusable = enabled(), .focused = focused(), .toggle = toggle}};
    }

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void toggle();

    CheckboxOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    motion::Animated check_fraction_;
    motion::Animated gravitation_;
    motion::Animated checked_;
    motion::Animated state_layer_;
    double snap_off_in_ = -1.0;
};

}
