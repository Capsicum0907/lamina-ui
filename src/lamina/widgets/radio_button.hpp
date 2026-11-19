#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/radio_button.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct RadioButtonOptions {
    bool selected = false;
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

    bool handle(const input::PointerEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    RadioButtonOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    motion::Animated dot_radius_;
    motion::Animated selected_;
    motion::Animated state_layer_;
};

}
