#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/card.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/interaction_elevation.hpp"
#include "lamina/widgets/widget_set.hpp"

namespace lamina::widgets {

struct CardOptions {
    tokens::CardStyle style = tokens::CardStyle::filled;
    bool clickable = false;
    bool enabled = true;
    layout::NodePtr content = nullptr;
    WidgetSet* widgets = nullptr;
    std::string accessible_name = {};
};

class Card {
public:
    explicit Card(CardOptions options);
    bool holds_pointer() const;

    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& r);
    render::Rect bounds() const { return rect_; }
    double radius() const;

    bool clickable() const { return options_.clickable; }
    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return options_.clickable && enabled(); }
    double elevation() const { return elevation_.value(); }
    double state_layer() const { return state_layer_.value(); }
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    bool over_content_control(double x, double y) const;
    void retarget();

    CardOptions options_;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    InteractionElevation elevation_;
    motion::Animated state_layer_;
    StyleStamp style_stamp_;
};

}
