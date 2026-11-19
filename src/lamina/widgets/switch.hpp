#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/tokens/switch.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct SwitchOptions {
    bool checked = false;
    std::optional<icons::Icon> checked_icon = std::nullopt;
    std::optional<icons::Icon> unchecked_icon = std::nullopt;
};

class Switch {
public:
    explicit Switch(SwitchOptions options = {});

    double width() const;
    double height() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;

    bool checked() const { return options_.checked; }
    void set_checked(bool checked);
    double thumb_size() const { return thumb_size_.value(); }
    double thumb_center_x() const;

    bool handle(const input::PointerEvent& event);
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    const std::optional<icons::Icon>& current_icon() const;
    double target_thumb_size() const;
    void retarget();

    SwitchOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    motion::Animated offset_;
    motion::Animated thumb_size_;
    motion::Animated state_layer_;
    motion::Animated checked_;
};

}
