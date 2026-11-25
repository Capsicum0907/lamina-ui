#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/animated_corners.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct ButtonColors {
    std::optional<color::Role> container;
    color::Role content;
    std::optional<color::Role> focus_ring = std::nullopt;
};

struct ButtonShapes {
    tokens::Corners shape;
    tokens::Corners pressed;
    std::optional<tokens::Corners> checked = std::nullopt;
    motion::SpringSpec spring = current_style().button.toggle_motion.shape;
};

struct ButtonToggle {
    bool checked = false;
    bool radio = false;
    bool checked_colors = true;
    double checked_overlay = 0.0;
    bool fill_checked_icon = true;
};

struct ButtonPadding {
    double leading;
    double trailing;
};

ButtonShapes toggle_button_shapes(tokens::ButtonSize size);

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
    std::optional<ButtonToggle> toggle = std::nullopt;
    std::optional<ButtonShapes> shapes = std::nullopt;
    bool center_optically = false;
    std::optional<ButtonPadding> padding = std::nullopt;
    std::optional<double> min_width = std::nullopt;
    std::optional<double> icon_size = std::nullopt;
    tokens::Density density = {};
    bool ellipsize_label = false;
};

class Button {
public:
    Button(ButtonOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const { return height_; }
    double intrinsic_height() const;
    void set_size(double width, double height);
    tokens::Density density() const { return options_.density; }
    void set_density(tokens::Density density);
    void set_label(std::string label);
    void set_content_scale(double label_scale, double icon_scale);
    void set_shape(tokens::ButtonShape shape);
    void set_icon(std::optional<icons::Icon> icon);
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    double intrinsic_width() const;
    double min_width() const;
    bool label_ellipsized() const { return ellipsized_; }
    const text::TextLine& shown_label() const { return ellipsized_ ? shown_ : label_; }
    render::CornerRadii corner_radii() const;
    double content_offset() const;
    ButtonPadding padding() const;
    double icon_size() const;
    void set_icon_rotation(double radians) { icon_rotation_ = radians; }
    void set_shapes(ButtonShapes shapes);

    bool pressed() const { return press_.pressed; }
    bool toggleable() const { return options_.toggle.has_value(); }
    bool checked() const { return options_.toggle && options_.toggle->checked; }
    void set_checked(bool checked);

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    const std::string& accessible_name() const { return options_.accessible_name.empty() ? options_.label : options_.accessible_name; }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    bool hits(const input::PointerEvent& event) const;
    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    double content_width() const;
    double content_width_for(double label_width) const;
    void fit_label();
    double rest_corner() const;
    double pressed_corner() const;
    double corner() const;
    void draw_icon(render::Canvas& canvas, const icons::Icon& icon, double x, double y, double size, color::Argb color,
                   double opacity, const icons::IconStyle& style) const;
    double target_state_layer() const;
    double target_elevation() const;
    void retarget();
    tokens::Corners target_corners() const;
    void activate();
    void paint_outline(render::Canvas& canvas, const render::Rect& r, const render::CornerRadii& radii, double width,
                       color::Argb color) const;

    ButtonOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    text::TextLine shown_;
    bool ellipsized_ = false;
    double label_room_ = -1.0;
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
    std::optional<AnimatedCorners> corners_;
    motion::Animated checked_amount_;
    motion::Animated border_;
    double icon_rotation_ = 0.0;
    bool style_shapes_ = false;
    StyleStamp style_stamp_;
};

}
