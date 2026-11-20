#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/icon_button.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct IconButtonOptions {
    icons::Icon icon;
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

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }

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
    motion::Animated icon_fade_;
};

}
