#pragma once

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/platform/input.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct FabOptions {
    icons::Icon icon;
    tokens::FabSize size = tokens::FabSize::baseline;
    tokens::FabStyle style = tokens::FabStyle::primary_container;
};

class Fab {
public:
    explicit Fab(FabOptions options);

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool handle(const platform::PointerEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double target_elevation() const;

    FabOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    motion::Animated state_layer_;
    motion::Animated elevation_;
};

}
