#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/image.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/density.hpp"
#include "lamina/tokens/list.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/enablement.hpp"
#include "lamina/widgets/focus_indicator.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/radio_button.hpp"
#include "lamina/widgets/switch.hpp"

namespace lamina::widgets {

enum class ListSlotKind { none, icon, avatar, image, checkbox, switch_control, radio_button, text };

struct ListSlot {
    ListSlotKind kind = ListSlotKind::none;
    std::optional<icons::Icon> icon = std::nullopt;
    std::string text = {};
    std::optional<render::Image> image = std::nullopt;

    static ListSlot of_icon(icons::Icon i) { return {ListSlotKind::icon, std::move(i)}; }
    static ListSlot of_avatar(std::string initials) { return {ListSlotKind::avatar, std::nullopt, std::move(initials)}; }
    static ListSlot of_avatar(render::Image image) { return {ListSlotKind::avatar, std::nullopt, {}, std::move(image)}; }
    static ListSlot of_image(render::Image image) { return {ListSlotKind::image, std::nullopt, {}, std::move(image)}; }
    static ListSlot of_text(std::string text) { return {ListSlotKind::text, std::nullopt, std::move(text)}; }
    static ListSlot checkbox() { return {ListSlotKind::checkbox}; }
    static ListSlot switch_control() { return {ListSlotKind::switch_control}; }
    static ListSlot radio_button() { return {ListSlotKind::radio_button}; }
};

enum class ListItemAction { none, click, select, toggle };

enum class ListLines { one, two, three };

struct ListItemSpec {
    std::string headline;
    std::string supporting = {};
    std::string overline = {};
    ListSlot leading = {};
    ListSlot trailing = {};
    ListItemAction action = ListItemAction::click;
    bool selected = false;
    bool enabled = true;
    std::string accessible_name = {};
    bool transparent = false;
    tokens::Density density = {};
};

struct ListItemResponse {
    bool clicked = false;
    bool consumed = false;
};

double list_item_estimate(const ListItemSpec& spec);

class ListItem {
public:
    ListItem(ListItemSpec spec, const text::FontStack& fonts);

    layout::Size measure(const layout::Constraints& c);
    double height_for(double width);
    void arrange(const render::Rect& r);
    render::Rect bounds() const { return rect_; }
    ListLines lines() const { return lines_; }
    tokens::Density density() const { return spec_.density; }
    void set_density(tokens::Density density);
    render::Rect leading_rect() const;
    render::Rect trailing_rect() const;

    const ListItemSpec& spec() const { return spec_; }
    bool selected() const { return spec_.selected; }
    void set_selected(bool selected);
    bool interactive() const { return spec_.action != ListItemAction::none; }
    double corner() const { return corner_.value(); }
    double selection() const { return selected_.value(); }

    bool focused() const { return focus_.focused(); }
    void set_focused(bool focused);
    bool enabled() const { return enabled_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return interactive() && enabled(); }
    bool hovered() const { return press_.hovered; }
    bool pressed() const { return press_.pressed; }
    AccessibleNode accessible() const;

    ListItemResponse handle(const input::PointerEvent& event);
    ListItemResponse handle(const input::KeyEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Slot {
        ListSlot spec;
        std::optional<text::TextLine> text;
        std::optional<Checkbox> checkbox;
        std::optional<Switch> switch_control;
        std::optional<RadioButton> radio;
        render::Rect rect{0.0, 0.0, 0.0, 0.0};
    };

    void build_slot(Slot& slot, const ListSlot& spec);
    static layout::Size slot_size(const Slot& slot);
    void sync_controls();
    void place_slot(Slot& slot, double x, double top, double inner, bool top_aligned);
    void paint_slot(render::Canvas& canvas, const color::ColorScheme& scheme, const Slot& slot, color::Argb color) const;
    tokens::TypeRole headline_role() const;
    void layout_text(double width);
    double target_corner() const;
    void retarget();
    void activate(ListItemResponse& response);

    ListItemSpec spec_;
    text::FontStack fonts_;
    Slot leading_;
    Slot trailing_;
    text::TextBlock headline_;
    text::TextBlock supporting_;
    std::optional<text::TextLine> overline_;
    double text_width_ = -1.0;
    std::optional<tokens::TypeRole> headline_role_;
    ListLines lines_ = ListLines::one;
    double height_ = 0.0;
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
    render::Point text_at_{0.0, 0.0};
    PressState press_;
    FocusIndicator focus_;
    Enablement enabled_;
    motion::Animated selected_;
    motion::Animated corner_;
    motion::Animated state_layer_;
    StyleStamp style_stamp_;
};

}
