#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/animated_radii.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/grouped_menu.hpp"
#include "lamina/widgets/interaction.hpp"
#include "lamina/widgets/popup_transition.hpp"

namespace lamina::widgets {

enum class MenuSelection { none, single, multiple };

struct GroupedMenuItem {
    std::string label;
    std::optional<icons::Icon> leading_icon = std::nullopt;
    std::optional<icons::Icon> selected_leading_icon = std::nullopt;
    std::optional<icons::Icon> trailing_icon = std::nullopt;
    bool selected = false;
};

struct MenuGroup {
    std::vector<GroupedMenuItem> items;
    std::string label = {};
    bool divider = false;
    MenuSelection selection = MenuSelection::none;
};

struct GroupedMenuOptions {
    std::vector<MenuGroup> groups;
};

struct MenuItemIndex {
    std::size_t group;
    std::size_t item;
};

struct GroupedMenuResponse {
    bool consumed = false;
    std::optional<MenuItemIndex> chosen = std::nullopt;
};

tokens::Corners menu_group_corners(std::size_t index, std::size_t count);
tokens::Corners menu_item_corners(std::size_t index, std::size_t count);

class GroupedMenu {
public:
    GroupedMenu(GroupedMenuOptions options, const text::FontStack& fonts);

    double width() const { return width_; }
    double height() const;
    render::Rect bounds() const;
    std::size_t group_count() const { return groups_.size(); }
    std::size_t item_count(std::size_t group) const { return groups_[group].items.size(); }
    render::Rect group_rect(std::size_t group) const;
    render::Rect item_rect(std::size_t group, std::size_t item) const;
    render::CornerRadii group_radii(std::size_t group) const;
    render::CornerRadii item_radii(std::size_t group, std::size_t item) const;

    bool selected(std::size_t group, std::size_t item) const { return groups_[group].items[item].selected; }
    void set_selected(std::size_t group, std::size_t item, bool selected);

    void open(const render::Rect& anchor, layout::Size window);
    void close();
    bool is_open() const { return popup_.is_open(); }
    bool visible() const { return popup_.visible(); }
    double scale() const { return popup_.scale(); }
    double opacity() const { return popup_.opacity(); }
    render::Point transform_origin() const { return pivot_; }

    GroupedMenuResponse handle(const input::PointerEvent& event);
    bool handle(const input::KeyEvent& event);
    void cancel_interaction();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    struct Item {
        std::optional<text::TextLine> label;
        std::optional<icons::Icon> leading_icon;
        std::optional<icons::Icon> selected_leading_icon;
        std::optional<icons::Icon> trailing_icon;
        bool selected;
        double y;
        double height;
        PressState press;
        motion::Animated state_layer;
        motion::Animated selection;
        motion::AnimatedRadii corners;
    };

    struct Group {
        std::optional<text::TextLine> label;
        double label_y;
        double label_height;
        std::optional<double> divider_y;
        MenuSelection selection;
        std::vector<Item> items;
        double y;
        double height;
        bool hovered;
        bool has_been_hovered;
        motion::AnimatedRadii corners;
    };

    render::CornerRadii group_target(std::size_t group) const;
    render::CornerRadii item_target(std::size_t group, std::size_t item) const;
    void retarget_group(std::size_t group);
    void retarget_item(std::size_t group, std::size_t item);
    void choose(std::size_t group, std::size_t item);

    text::FontStack fonts_;
    std::vector<Group> groups_;
    double width_ = 0.0;
    render::Point origin_{0.0, 0.0};
    render::Point pivot_{0.0, 0.0};
    PopupTransition popup_;
};

}
