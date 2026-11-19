#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/icon_button.hpp"
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

    bool handle(const input::PointerEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double target_corner() const;
    void retarget();

    IconButtonOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    motion::Animated corner_;
    motion::Animated state_layer_;
    motion::Animated checked_;
};

}
