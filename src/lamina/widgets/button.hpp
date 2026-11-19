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

struct ButtonColors {
    color::Role container;
    color::Role content;
};

struct ButtonOptions {
    std::string label;
    tokens::ButtonStyle style = tokens::ButtonStyle::filled;
    tokens::ButtonSize size = tokens::ButtonSize::small;
    tokens::ButtonShape shape = tokens::ButtonShape::round;
    std::optional<icons::Icon> leading_icon = std::nullopt;
    std::optional<ButtonColors> colors = std::nullopt;
    icons::IconStyle icon_style = {};
};

class Button {
public:
    Button(ButtonOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    void set_size(double width, double height);
    void set_label(std::string label);
    void set_content_scale(double label_scale, double icon_scale);
    void set_shape(tokens::ButtonShape shape);
    void set_icon(std::optional<icons::Icon> icon);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool handle(const input::PointerEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double content_width() const;
    double intrinsic_width() const;
    double rest_corner() const;
    double pressed_corner() const;
    double corner() const;
    void draw_icon(render::Canvas& canvas, const icons::Icon& icon, double x, double y, double size, color::Argb color,
                   double opacity) const;
    double target_state_layer() const;
    void retarget();

    ButtonOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    double width_;
    double height_;
    bool sized_ = false;
    double label_scale_ = 1.0;
    double icon_scale_ = 1.0;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    std::optional<icons::Icon> previous_icon_;
    motion::Animated press_amount_;
    motion::Animated squareness_;
    motion::Animated icon_fade_;
    motion::Animated state_layer_;
    motion::Animated elevation_;
};

}
