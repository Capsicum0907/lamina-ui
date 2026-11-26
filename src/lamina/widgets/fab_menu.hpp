#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/fab_menu.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/interaction.hpp"

namespace lamina::widgets {

struct ToggleFabOptions {
    icons::Icon icon;
    icons::Icon checked_icon;
    std::string accessible_name;
    tokens::FabSize size = tokens::FabSize::baseline;
    bool checked = false;
};

class ToggleFab {
public:
    explicit ToggleFab(ToggleFabOptions options);

    double width() const { return current_style().fab.size(options_.size).size; }
    double height() const { return width(); }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }
    render::Rect container() const;
    double corner_radius() const;
    double icon_size() const;
    double checked_progress() const { return progress_.value(); }

    bool checked() const { return options_.checked; }
    void set_checked(bool checked);

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool can_focus() const { return true; }
    const std::string& accessible_name() const { return options_.accessible_name; }
    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const;

    bool handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void retarget();

    ToggleFabOptions options_;
    render::Point origin_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    motion::Animated progress_;
    motion::Animated state_layer_;
    StyleStamp style_stamp_;
};

struct FabMenuItem {
    std::string label;
    icons::Icon icon;
};

enum class FabMenuAlignment { start, end };

struct FabMenuOptions {
    std::vector<FabMenuItem> items;
    ToggleFabOptions button;
    FabMenuAlignment alignment = FabMenuAlignment::end;
    std::string accessible_name = {};
};

struct FabMenuResponse {
    bool toggled = false;
    std::optional<std::size_t> chosen = std::nullopt;
    bool consumed = false;
};

class FabMenu {
public:
    FabMenu(FabMenuOptions options, const text::FontStack& fonts);

    double width() const;
    double height() const;
    void set_origin(render::Point origin) {
        origin_ = origin;
        place_button();
    }
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }

    ToggleFab& button() { return button_; }
    const ToggleFab& button() const { return button_; }
    bool expanded() const { return button_.checked(); }
    void set_expanded(bool expanded);
    std::size_t size() const { return items_.size(); }
    std::size_t visible_count() const;
    double item_width_progress(std::size_t index) const { return items_[index].width.value(); }
    double item_alpha(std::size_t index) const { return items_[index].alpha.value(); }
    render::Rect item_rect(std::size_t index) const;
    bool item_shown(std::size_t index) const { return items_[index].alpha.value() > 0.0; }
    const std::string& item_name(std::size_t index) const { return items_[index].spec.label; }
    std::vector<FocusStop*> focus_stops();
    std::optional<std::size_t> focused_item() const;

    AccessibleNode accessible() const;

    FabMenuResponse handle(const input::PointerEvent& event);
    FabMenuResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        FabMenuItem spec;
        text::TextLine label;
        double full_width = 0.0;
        PressState press;
        FocusIndicator focus;
        motion::Animated state_layer;
        motion::Animated width;
        motion::Animated alpha;
        bool target = false;
    };

    double items_width() const;
    double items_height() const;
    void retarget_items();
    void place_button();
    void layout_item(Item& item) const;

    FabMenuOptions options_;
    text::FontStack fonts_;
    ToggleFab button_;
    std::vector<Item> items_;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    motion::Animated stagger_;
    render::Point origin_{0.0, 0.0};
    StyleStamp style_stamp_;
};

}
