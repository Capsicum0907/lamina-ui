#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/caption.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/label.hpp"

namespace lamina::widgets {

struct TitleBarNames {
    std::string minimize = "Minimize";
    std::string maximize = "Maximize";
    std::string restore = "Restore";
    std::string close = "Close";
};

struct TitleBarOptions {
    std::string title;
    icons::IconFont icons;
    layout::NodePtr content = nullptr;
    std::optional<color::Role> container = std::nullopt;
    TitleBarNames names = {};
};

enum class TitleBarAction { none, minimize, toggle_maximize, close };

class TitleBar {
public:
    TitleBar(TitleBarOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin);
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height()}; }

    void set_title(std::string title);
    void set_maximized(bool maximized) { maximized_ = maximized; }
    void set_active(bool active);
    bool maximized() const { return maximized_; }

    render::Rect button_rect(TitleBarAction action) const;
    input::CaptionRegions regions(std::span<const render::Rect> interactive = {}) const;

    TitleBarAction handle(const input::PointerEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    AccessibleNode accessible() const;

private:
    struct Button {
        TitleBarAction action;
        PressState press;
        motion::Animated state_layer;
        motion::Animated emphasis;
        motion::Animated corner;
    };

    void retarget(Button& button);
    void paint_glyph(render::Canvas& canvas, const render::Rect& indicator, TitleBarAction action, color::Argb color) const;
    void paint_button(render::Canvas& canvas, const color::ColorScheme& scheme, const Button& button) const;

    TitleBarOptions options_;
    Label title_;
    std::array<Button, 3> buttons_;
    motion::Animated active_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    bool maximized_ = false;
};

}
