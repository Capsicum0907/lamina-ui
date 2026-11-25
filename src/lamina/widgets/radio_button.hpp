#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include <optional>
#include <string>

#include "lamina/tokens/radio_button.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct RadioButtonOptions {
    bool selected = false;
    bool enabled = true;
    std::string accessible_name = {};
};

class RadioButton {
public:
    explicit RadioButton(RadioButtonOptions options = {});

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool selected() const { return options_.selected; }
    void set_selected(bool selected);
    double dot_radius() const { return dot_radius_.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const {
        return {AccessibleRole::radio_button, options_.accessible_name, bounds(),
                {.enabled = enabled(), .focusable = enabled(), .focused = focused(), .selected = options_.selected}};
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
    RadioButtonOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    motion::Animated dot_radius_;
    motion::Animated selected_;
    motion::Animated state_layer_;
    StyleStamp style_stamp_;
};

}
