#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct ButtonOptions {
    std::string label;
    tokens::ButtonStyle style = tokens::ButtonStyle::filled;
    tokens::ButtonSize size = tokens::ButtonSize::small;
    tokens::ButtonShape shape = tokens::ButtonShape::round;
    std::optional<icons::Icon> leading_icon = std::nullopt;
};

class Button {
public:
    Button(ButtonOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    void set_size(double width, double height);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool handle(const input::PointerEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double content_width() const;
    double rest_corner() const;
    double pressed_corner() const;
    double target_state_layer() const;
    void retarget();

    ButtonOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    double width_;
    double height_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    motion::Animated corner_;
    motion::Animated state_layer_;
    motion::Animated elevation_;
};

}
