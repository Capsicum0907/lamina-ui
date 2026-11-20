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
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct ButtonColors {
    std::optional<color::Role> container;
    color::Role content;
    std::optional<color::Role> focus_ring = std::nullopt;
};

struct ButtonOptions {
    std::string label;
    tokens::ButtonStyle style = tokens::ButtonStyle::filled;
    tokens::ButtonSize size = tokens::ButtonSize::small;
    tokens::ButtonShape shape = tokens::ButtonShape::round;
    std::optional<icons::Icon> leading_icon = std::nullopt;
    std::optional<ButtonColors> colors = std::nullopt;
    icons::IconStyle icon_style = {};
    bool enabled = true;
    std::string accessible_name = {};
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

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    const std::string& accessible_name() const { return options_.accessible_name.empty() ? options_.label : options_.accessible_name; }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const {
        return {AccessibleRole::button, accessible_name(), bounds(), {.enabled = enabled(), .focusable = enabled(), .focused = focused()}};
    }

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
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
    double target_elevation() const;
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
    FocusIndicator focus_;
    Enablement enabled_;
    std::optional<icons::Icon> previous_icon_;
    motion::Animated press_amount_;
    motion::Animated squareness_;
    motion::Animated icon_fade_;
    motion::Animated state_layer_;
    motion::Animated elevation_;
};

}
