#pragma once

#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/fab.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct ExtendedFabOptions {
    std::string label;
    icons::Icon icon;
    tokens::ExtendedFabSize size = tokens::ExtendedFabSize::small;
    tokens::FabStyle style = tokens::FabStyle::primary_container;
    bool expanded = true;
};

class ExtendedFab {
public:
    ExtendedFab(ExtendedFabOptions options, const text::FontStack& fonts);

    double width() const;
    double height() const { return tokens::extended_fab_size(options_.size).height; }
    double expanded_width() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }

    bool expanded() const { return options_.expanded; }
    void set_expanded(bool expanded);
    double label_alpha() const { return label_alpha_.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    const std::string& accessible_name() const { return options_.label; }
    AccessibleNode accessible() const {
        return {AccessibleRole::button, options_.label, bounds(), {.focusable = true, .focused = focused()}};
    }

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void retarget();

    ExtendedFabOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    motion::Animated width_progress_;
    motion::Animated label_alpha_;
    motion::Animated state_layer_;
    motion::Animated elevation_;
};

}
