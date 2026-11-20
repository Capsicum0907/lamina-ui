#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/image.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/chip.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/interaction_elevation.hpp"

namespace lamina::widgets {

struct ChipOptions {
    std::string label;
    tokens::ChipKind kind = tokens::ChipKind::assist;
    bool elevated = false;
    std::optional<icons::Icon> leading_icon = std::nullopt;
    std::optional<icons::Icon> selected_leading_icon = std::nullopt;
    std::optional<icons::Icon> trailing_icon = std::nullopt;
    std::optional<render::Image> avatar = std::nullopt;
    bool removable = false;
    bool selected = false;
    bool enabled = true;
    std::string accessible_name = {};
    std::string remove_name = "Remove";
};

struct ChipResponse {
    bool clicked = false;
    bool removed = false;
    bool consumed = false;
};

class Chip {
public:
    Chip(ChipOptions options, const text::FontStack& fonts);

    double width() const;
    double height() const;
    double target_width() const;
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const;
    std::optional<render::Rect> leading_rect() const;
    std::optional<render::Rect> trailing_rect() const;
    std::optional<render::Rect> remove_target() const;

    const ChipOptions& options() const { return options_; }
    const std::string& label() const { return options_.label; }
    bool selected() const { return options_.selected; }
    void set_selected(bool selected);
    double selection() const { return selected_.value(); }
    double leading_presence() const { return leading_size_.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return enabled(); }
    double elevation() const { return elevation_.value(); }
    const std::string& accessible_name() const { return options_.accessible_name.empty() ? options_.label : options_.accessible_name; }
    AccessibleNode accessible() const;

    ChipResponse handle(const input::PointerEvent& event);
    ChipResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    const std::optional<icons::Icon>& shown_leading() const;
    bool has_leading() const;
    double leading_width() const;
    double start_padding() const;
    double end_padding() const;
    double trailing_width() const;
    double content_width(double leading_fraction) const;
    void retarget();
    void update_leading();
    void activate(ChipResponse& response);

    ChipOptions options_;
    text::FontStack fonts_;
    text::TextLine label_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    PressState remove_press_;
    FocusIndicator focus_;
    Enablement enabled_;
    InteractionElevation elevation_;
    motion::Animated selected_;
    motion::Animated state_layer_;
    motion::Animated leading_size_;
    motion::Animated leading_alpha_;
    std::optional<icons::Icon> exiting_leading_;
};

}
