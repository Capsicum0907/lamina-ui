#pragma once

#include <optional>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/anchored_draggable.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/navigation_rail.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/navigation_rail.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

enum class RailValue { collapsed, expanded };

struct ModalNavigationRailOptions {
    NavigationRailOptions rail;
    bool hide_on_collapse = false;
    const PatternStrings* strings = &english_us_patterns();
};

struct ModalRailResponse {
    bool selected = false;
    bool collapsed = false;
    bool consumed = false;
};

class ModalNavigationRail {
public:
    ModalNavigationRail(ModalNavigationRailOptions options, const text::FontStack& fonts);
    ModalNavigationRail(const ModalNavigationRail&) = delete;
    ModalNavigationRail& operator=(const ModalNavigationRail&) = delete;

    layout::Size measure(const layout::Constraints& c);
    void arrange(const render::Rect& rect);
    void set_area(const render::Rect& area);
    render::Rect bounds() const { return rail_.bounds(); }
    render::Rect area() const { return area_; }
    NavigationRail& rail() { return rail_; }
    const NavigationRail& rail() const { return rail_; }

    void expand(input::InputSource source = input::InputSource::pointer);
    void collapse();
    bool expanded() const { return target_expanded_; }
    bool is_open() const { return showing(); }
    bool showing() const;
    double scrim_alpha() const { return std::clamp(scrim_.value(), 0.0, 1.0); }
    double offset() const { return slide_.offset(); }
    bool hide_on_collapse() const { return options_.hide_on_collapse; }

    bool focused() const { return rail_.focused(); }
    void set_focused(bool focused) { rail_.set_focused(focused && (!options_.hide_on_collapse || showing())); }
    bool can_focus() const { return rail_.can_focus() && (!options_.hide_on_collapse || showing()); }
    bool enabled() const { return rail_.enabled(); }
    void set_enabled(bool enabled) { rail_.set_enabled(enabled); }

    ModalRailResponse handle(const input::PointerEvent& event);
    ModalRailResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    void layout_rail();

    ModalNavigationRailOptions options_;
    NavigationRail rail_;
    motion::AnchoredDraggable<RailValue> slide_;
    motion::Animated scrim_;
    render::Rect slot_{0.0, 0.0, 0.0, 0.0};
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    bool target_expanded_ = false;
    bool tracking_ = false;
    bool scrim_pressed_ = false;
    bool claim_focus_ = false;
};

}
