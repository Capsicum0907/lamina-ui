#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/button.hpp"
#include "lamina/tokens/button_group.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_stop.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina/widgets/pattern_strings.hpp"

namespace lamina::widgets {

enum class ButtonGroupSelection { none, single, multiple };

struct ButtonGroupItem {
    std::string label;
    std::optional<icons::Icon> icon = std::nullopt;
    bool checked = false;
    bool enabled = true;
    std::string accessible_name = {};
    double weight = 0.0;
};

struct ButtonGroupOptions {
    std::vector<ButtonGroupItem> items;
    ButtonGroupSelection selection = ButtonGroupSelection::none;
    bool connected = false;
    tokens::ButtonStyle style = tokens::ButtonStyle::filled;
    tokens::ButtonSize size = tokens::ButtonSize::small;
    double expanded_ratio = tokens::button_group_tokens.expanded_ratio;
    bool animate_width = true;
    std::string accessible_name = {};
    bool enabled = true;
    const PatternStrings* strings = &english_us_patterns();
};

struct ButtonGroupResponse {
    std::optional<std::size_t> clicked = std::nullopt;
    bool consumed = false;
};

class ButtonGroup {
public:
    ButtonGroup(ButtonGroupOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font);

    layout::Size measure(const layout::Constraints& constraints);
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return bounds_; }
    void set_window(layout::Size window) { window_ = window; }

    std::size_t size() const { return items_.size(); }
    std::size_t visible_count() const { return visible_; }
    bool overflowing() const { return visible_ < items_.size(); }
    Button& button(std::size_t index) { return *items_[index].button; }
    const Button& button(std::size_t index) const { return *items_[index].button; }
    IconButton& overflow_button() { return *overflow_; }
    const IconButton& overflow_button() const { return *overflow_; }
    const Menu* menu() const { return menu_.get(); }
    bool menu_open() const { return menu_ && menu_->is_open(); }
    double press_progress(std::size_t index) const { return items_[index].pressed.value(); }
    double item_width(std::size_t index) const { return items_[index].width; }
    bool checked(std::size_t index) const { return items_[index].button->checked(); }
    void set_checked(std::size_t index, bool checked);
    std::optional<std::size_t> selected() const;
    const std::string& item_name(std::size_t index) const { return items_[index].button->accessible_name(); }
    std::vector<FocusStop*> focus_stops();

    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    AccessibleNode accessible() const;

    std::optional<TargetHit> target_hit(const input::PointerEvent& event) const;
    ButtonGroupResponse handle(const input::PointerEvent& event);
    ButtonGroupResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_overlay(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    template <typename Offer>
    void offer_targets(const input::PointerEvent& event, Offer offer) const;
    struct Item {
        ButtonGroupItem spec;
        std::unique_ptr<Button> button;
        motion::Animated pressed;
        bool releasing = false;
        bool was_pressed = false;
        double base = 0.0;
        double width = 0.0;
    };

    double spacing() const;
    double compression_limit(std::size_t index) const;
    void layout_items();
    void choose(std::size_t index, ButtonGroupResponse& response);
    void open_menu(input::InputSource source);
    void track_press(Item& item);

    ButtonGroupOptions options_;
    text::FontStack fonts_;
    icons::IconFont icon_font_;
    std::vector<Item> items_;
    std::unique_ptr<IconButton> overflow_;
    TargetArbiter arbiter_;
    std::unique_ptr<Menu> menu_;
    std::vector<std::size_t> menu_items_;
    std::vector<std::unique_ptr<FocusStop>> stops_;
    std::size_t visible_ = 0;
    double max_width_ = 0.0;
    render::Rect bounds_{0.0, 0.0, 0.0, 0.0};
    layout::Size window_{0.0, 0.0};
    bool enabled_ = true;
    bool reopen_blocked_ = false;
};

}
